package main

import "core:fmt"
import "core:path/filepath"
import "core:strings"
import rl "vendor:raylib"

// -----------------------------------------------------------------------------
// Constants & Theme (Classic 90s)
// -----------------------------------------------------------------------------

// Palette: Windows 95 / Classic Mac Style
COL_BG :: rl.Color{192, 192, 192, 255} // Standard Gray
COL_WINDOW_TEXT :: rl.Color{0, 0, 0, 255} // Black
COL_TITLE_BG :: rl.Color{0, 0, 128, 255} // Classic Blue
COL_TITLE_TEXT :: rl.Color{255, 255, 255, 255} // White
COL_INPUT_BG :: rl.Color{255, 255, 255, 255} // White
COL_LIGHT :: rl.Color{255, 255, 255, 255} // Bevel Highlight
COL_SHADOW :: rl.Color{128, 128, 128, 255} // Bevel Shadow
COL_DARK_SHADOW :: rl.Color{0, 0, 0, 255} // Deep Shadow (Borders)

SCREEN_WIDTH :: 700
SCREEN_HEIGHT :: 400

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

// Case-insensitive prefix check
starts_with_ci :: proc(s, prefix: string) -> bool {
	if len(prefix) > len(s) do return false
	for i in 0 ..< len(prefix) {
		c1 := s[i]
		c2 := prefix[i]
		// Simple ASCII lowercasing
		if c1 >= 'A' && c1 <= 'Z' do c1 += 32
		if c2 >= 'A' && c2 <= 'Z' do c2 += 32
		if c1 != c2 do return false
	}
	return true
}

// Draw a classic 3D Bevel Box
draw_bevel_box :: proc(rect: rl.Rectangle, raised: bool, fill_color: rl.Color) {
	x := i32(rect.x)
	y := i32(rect.y)
	w := i32(rect.width)
	h := i32(rect.height)

	// Fill
	rl.DrawRectangleRec(rect, fill_color)

	// Colors
	c_tl_outer := raised ? COL_LIGHT : COL_SHADOW
	c_tl_inner := raised ? COL_BG : COL_DARK_SHADOW
	c_br_inner := raised ? COL_SHADOW : COL_BG
	c_br_outer := raised ? COL_DARK_SHADOW : COL_LIGHT

	// Top/Left Outer
	rl.DrawLine(x, y, x + w - 1, y, c_tl_outer)
	rl.DrawLine(x, y, x, y + h - 1, c_tl_outer)

	// Top/Left Inner
	rl.DrawLine(x + 1, y + 1, x + w - 2, y + 1, c_tl_inner)
	rl.DrawLine(x + 1, y + 1, x + 1, y + h - 2, c_tl_inner)

	// Bottom/Right Inner
	rl.DrawLine(x + 1, y + h - 2, x + w - 2, y + h - 2, c_br_inner)
	rl.DrawLine(x + w - 2, y + 1, x + w - 2, y + h - 2, c_br_inner)

	// Bottom/Right Outer
	rl.DrawLine(x, y + h - 1, x + w, y + h - 1, c_br_outer)
	rl.DrawLine(x + w - 1, y, x + w - 1, y + h, c_br_outer)
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

main :: proc() {
	rl.SetConfigFlags({.WINDOW_UNDECORATED, .WINDOW_TRANSPARENT, .MSAA_4X_HINT})
	rl.InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "MaxSearch")
	defer rl.CloseWindow()

	refresh_rate := rl.GetMonitorRefreshRate(rl.GetCurrentMonitor())
	rl.SetTargetFPS(refresh_rate)

	// State
	search_query: [dynamic]byte
	defer delete(search_query)

	suggestion_suffix := ""
	suggestion_display_extra := ""
	frame_counter := 0
	is_dragging := false
	drag_anchor := rl.Vector2{}
	should_close := false 

	// Engine
	engine: SearchEngine
	se_init(&engine)
	defer se_destroy(&engine)

	// Geometry
	padding :: f32(6.0)
	title_height :: f32(24.0)

	// Layout Constants
	HEIGHT_COMPACT :: 74
	HEIGHT_EXPANDED :: 400

	current_window_h := i32(HEIGHT_COMPACT)
	rl.SetWindowSize(SCREEN_WIDTH, current_window_h)

	// Search bar geometry is static relative to top
	search_bar_y := padding + title_height + padding
	search_bar_height :: f32(32.0)
	search_bar_rect := rl.Rectangle {
		padding,
		search_bar_y,
		SCREEN_WIDTH - padding * 2,
		search_bar_height,
	}

	for !rl.WindowShouldClose() && !should_close {
		// Reset temp allocator at the start of each frame
		free_all(context.temp_allocator)

		// Update
		// ---------------------------------------------------------------------

		// Fetch latest results safely
		current_results := se_get_results(&engine)

		// Dynamic Resizing Logic
		target_h := i32(HEIGHT_COMPACT)
		if len(current_results) > 0 {
			target_h = HEIGHT_EXPANDED
		}

		if target_h != current_window_h {
			rl.SetWindowSize(SCREEN_WIDTH, target_h)
			current_window_h = target_h
		}

		// Update Dynamic Layout Rects
		main_rect := rl.Rectangle{0, 0, SCREEN_WIDTH, f32(current_window_h)}
		// Title rect is always at top
		title_rect := rl.Rectangle{padding, padding, SCREEN_WIDTH - padding * 2, title_height}

		results_y := search_bar_y + search_bar_height + padding
		results_h := f32(current_window_h) - results_y - padding
		results_rect := rl.Rectangle{padding, results_y, SCREEN_WIDTH - padding * 2, results_h}
		
		// Close "Button" Geometry
		close_btn_size :: 16.0
		close_btn_x := title_rect.x + title_rect.width - close_btn_size - 2
		close_btn_y := title_rect.y + (title_rect.height - close_btn_size) / 2
		close_btn_rect := rl.Rectangle{close_btn_x, close_btn_y, close_btn_size, close_btn_size}

		// Window Dragging (Only on Title Bar) & Close Button

		mouse_pos := rl.GetMousePosition()
		if rl.IsMouseButtonPressed(.LEFT) {
			if rl.CheckCollisionPointRec(mouse_pos, close_btn_rect) {
				should_close = true
			} else if rl.CheckCollisionPointRec(mouse_pos, title_rect) {
				is_dragging = true
				drag_anchor = mouse_pos
			}
		}
		if rl.IsMouseButtonReleased(.LEFT) {
			is_dragging = false
		}

		if is_dragging {
			delta := mouse_pos - drag_anchor
			// Standard windows dragging is usually rigid 1:1, but we can keep a tiny smoothing or go instant
			// For Retro feel, instant (stiffness 1.0) is more authentic, but let's keep it slightly smooth (0.8)
			stiffness :: 0.9

			if (delta.x * delta.x + delta.y * delta.y) > 1.0 {
				move := delta * stiffness
				wp := rl.GetWindowPosition()
				new_pos := rl.Vector2{f32(wp.x), f32(wp.y)} + move
				rl.SetWindowPosition(i32(new_pos.x), i32(new_pos.y))
			}
		}

	


		// Calculate Suggestion Suffix & Open Label
		suggestion_suffix = ""
		suggestion_display_extra = ""

		query_str := string(search_query[:])

		if len(search_query) > 0 && len(current_results) > 0 {
			first_path := current_results[0]
			filename := filepath.base(first_path)

			if starts_with_ci(filename, query_str) {
				suggestion_suffix = filename[len(query_str):]
			}
			if len(first_path) > 4 {
				ext := first_path[len(first_path) - 4:]
				if strings.to_lower(ext) == ".app" {
					suggestion_display_extra = " [APP]"
				}
			}
		}

		// Text Input
		text_changed := false

		// Handle Tab Completion
		if rl.IsKeyPressed(.TAB) && len(suggestion_suffix) > 0 {
			for c in suggestion_suffix {
				append(&search_query, byte(c))
			}
			text_changed = true
		}
		key := rl.GetCharPressed()
		for key > 0 {
			if key >= 32 && key <= 125 {
				append(&search_query, byte(key))
				text_changed = true
			}
			key = rl.GetCharPressed()
		}

		// handling the logic to delete input from the input box
		if rl.IsKeyPressed(.BACKSPACE) || rl.IsKeyPressedRepeat(.BACKSPACE) {
			if len(search_query) > 0 {
				if rl.IsKeyDown(.LEFT_ALT) || rl.IsKeyDown(.RIGHT_ALT) {
					for len(search_query) > 0 && search_query[len(search_query) - 1] == ' ' {
						pop(&search_query)
					}
					for len(search_query) > 0 && search_query[len(search_query) - 1] != ' ' {
						pop(&search_query)
					}
				} else {
					pop(&search_query)
				}
				text_changed = true
			}
		}

		if text_changed {
			se_search(&engine, string(search_query[:]))
		}

		frame_counter += 1

		// Draw
		// ---------------------------------------------------------------------
		rl.BeginDrawing()
		rl.ClearBackground(rl.BLANK) // Clear with transparent for the OS window context

		// 1. Main Window Body (Raised 3D)
		draw_bevel_box(main_rect, true, COL_BG)

		// 2. Title Bar (Blue gradient or solid)
		rl.DrawRectangleRec(title_rect, COL_TITLE_BG)
		rl.DrawText("MaxSearch", i32(title_rect.x) + 4, i32(title_rect.y) + 4, 10, COL_TITLE_TEXT) // Small retro font


		// Close "Button" (Visual only for now, just a box)
		draw_bevel_box(close_btn_rect, true, COL_BG)
		rl.DrawText("x", i32(close_btn_rect.x) + 5, i32(close_btn_rect.y) - 1, 10, COL_WINDOW_TEXT)
		

		// 3. Search Bar (Sunken 3D)
		draw_bevel_box(search_bar_rect, false, COL_INPUT_BG)

		// Text & Cursor
		text_x := i32(search_bar_rect.x + 8)
		text_y := i32(search_bar_rect.y + 6)
		font_size := i32(20)

		query_str = string(search_query[:])

		if len(search_query) == 0 {
			// Placeholder
		} else {
			c_query := strings.clone_to_cstring(query_str, context.temp_allocator)
			rl.DrawText(c_query, text_x, text_y, font_size, COL_WINDOW_TEXT)

			user_text_width := rl.MeasureText(c_query, font_size)

			// Ghost Text
			if len(suggestion_suffix) > 0 {
				c_suffix := strings.clone_to_cstring(suggestion_suffix, context.temp_allocator)
				rl.DrawText(c_suffix, text_x + user_text_width, text_y, font_size, COL_SHADOW)
				user_text_width += rl.MeasureText(c_suffix, font_size)
			}

			// [APP] Label
			if len(suggestion_display_extra) > 0 {
				c_extra := strings.clone_to_cstring(
					suggestion_display_extra,
					context.temp_allocator,
				)
				rl.DrawText(c_extra, text_x + user_text_width, text_y, font_size, COL_TITLE_BG)
			}
		}

		// Cursor (I-Beam or Block)

		if (frame_counter / 30) % 2 == 0 {
			cursor_x := text_x
			if len(search_query) > 0 {
				c_query := strings.clone_to_cstring(query_str, context.temp_allocator)
				cursor_x += rl.MeasureText(c_query, font_size)
			}
			// Classic "Thin Line" cursor
			rl.DrawRectangle(cursor_x + 1, text_y, 1, 20, COL_WINDOW_TEXT)
		}

		// 4. Results List (Sunken 3D) - Only draw if we have results
		if len(current_results) > 0 {
			draw_bevel_box(results_rect, false, COL_INPUT_BG)
			// List Items
			list_start_y := i32(results_rect.y) + 6
			item_height :: 20
			for res, i in current_results {
				y_pos := list_start_y + i32(i) * item_height
				if y_pos + item_height > i32(results_rect.y + results_rect.height) {
					break // Clip
				}
				// Simple Selection Highlight (First item is always "selected" concept)
				if i == 0 {
					// Dotted selection box or blue highlight? Classic Windows uses blue for selection.
					// Let's do a blue box
					highlight_rect := rl.Rectangle {
						results_rect.x + 2,
						f32(y_pos),
						results_rect.width - 4,
						f32(item_height),
					}
					rl.DrawRectangleRec(highlight_rect, COL_TITLE_BG)
				}
				// Text
				display_path := res
				// Truncate from middle or end? End is simpler
				if len(display_path) > 70 {
					suffix := display_path[len(display_path) - 67:]
					display_path = fmt.tprintf("...%s", suffix)
				}
				c_path := strings.clone_to_cstring(display_path, context.temp_allocator)
				text_col := (i == 0) ? COL_TITLE_TEXT : COL_WINDOW_TEXT
				rl.DrawText(c_path, i32(results_rect.x) + 6, y_pos + 2, 10, text_col)
			}
		}
		rl.EndDrawing()
	}
}
