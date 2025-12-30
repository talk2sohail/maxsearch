package main

import "core:fmt"
import "core:path/filepath"
import "core:strings"
import rl "vendor:raylib"

// -----------------------------------------------------------------------------
// Constants & Theme
// -----------------------------------------------------------------------------

COL_BAR_BG :: rl.Color{20, 25, 40, 200} // Deep Glass
COL_BORDER :: rl.Color{120, 140, 180, 80} // Soft Rim
COL_TEXT :: rl.Color{192, 202, 245, 255}
COL_ACCENT :: rl.Color{122, 162, 247, 255}
COL_ICON :: rl.Color{169, 177, 214, 200}
COL_GHOST :: rl.Color{169, 177, 214, 100}

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

	// Engine
	engine: SearchEngine
	se_init(&engine)
	defer se_destroy(&engine)

	// Geometry
	margin :: 10.0
	bar_width :: SCREEN_WIDTH - (margin * 2)
	bar_height :: 70.0 - (margin * 2)
	bar_x :: margin
	bar_y :: margin

	search_bar_rect := rl.Rectangle{bar_x, bar_y, bar_width, bar_height}

	for !rl.WindowShouldClose() {
		// Reset temp allocator at the start of each frame to avoid memory leaks
		free_all(context.temp_allocator)

		// Update
		// ---------------------------------------------------------------------

		// Fetch latest results safely
		current_results := se_get_results(&engine)

		// Window Dragging
		mouse_pos := rl.GetMousePosition()
		if rl.IsMouseButtonPressed(.LEFT) {
			if rl.CheckCollisionPointRec(mouse_pos, search_bar_rect) {
				is_dragging = true
				drag_anchor = mouse_pos
			}
		}
		if rl.IsMouseButtonReleased(.LEFT) {
			is_dragging = false
		}

		if is_dragging {
			// Physics-based Dragging (Spring/Lerp)
			// We calculate the distance (tension) between current mouse and our anchor
			delta := mouse_pos - drag_anchor

			// Stiffness: 0.1 = Very Loose/Heavy, 0.9 = Instant/Rigid
			// 0.6 Provides a good balance of responsiveness and smoothing
			stiffness :: 0.6

			// Deadzone: Prevents micro-jitter when holding still
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

			// 1. Check for Text Completion
			if starts_with_ci(filename, query_str) {
				suggestion_suffix = filename[len(query_str):]
			}

			// 2. Check for "Open" status (App detection)
			// Case-insensitive check for .app extension
			if len(first_path) > 4 {
				ext := first_path[len(first_path) - 4:]
				if strings.to_lower(ext) == ".app" {
					suggestion_display_extra = "  —  Open"
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

		if rl.IsKeyPressed(.BACKSPACE) || rl.IsKeyPressedRepeat(.BACKSPACE) {
			if len(search_query) > 0 {
				if rl.IsKeyDown(.LEFT_ALT) || rl.IsKeyDown(.RIGHT_ALT) {
					// Delete word (simple approx)
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
		rl.ClearBackground(rl.BLANK)

		// 1. Search Bar Background
		rl.DrawRectangleRounded(search_bar_rect, 0.8, 20, COL_BAR_BG)
		rl.DrawRectangleRoundedLines(search_bar_rect, 0.8, 20, COL_BORDER)

		// 2. Icon (Magnifying Glass)
		icon_center_x := i32(bar_x + 25)
		icon_center_y := i32(bar_y + bar_height / 2)
		radius := 8.5

		rl.DrawCircleLines(icon_center_x, icon_center_y, f32(radius), COL_ICON)
		rl.DrawCircleLines(icon_center_x, icon_center_y, f32(radius - 0.5), COL_ICON)

		start := rl.Vector2{f32(icon_center_x) + 5, f32(icon_center_y) + 5}
		end := rl.Vector2{f32(icon_center_x) + 12, f32(icon_center_y) + 12}
		rl.DrawLineEx(start, end, 3.0, COL_ICON)

		// 3. Text & Cursor
		base_text_x := i32(bar_x + 55)
		text_padding := i32(4)
		font_size := i32(28)
		cursor_height := i32(32)

		cursor_y_f := bar_y + (bar_height - f32(cursor_height)) / 2
		cursor_y_i := i32(cursor_y_f) + 1

		text_y_i := (cursor_y_i + (cursor_height - font_size) / 2) + 1

		// We update query_str again in case it changed via Tab
		query_str = string(search_query[:])

		if len(search_query) == 0 {
			rl.DrawText(
				"Max Search here",
				base_text_x + text_padding,
				text_y_i,
				font_size,
				rl.Fade(COL_TEXT, 0.5),
			)
		} else {
			// User Text
			c_query := strings.clone_to_cstring(query_str, context.temp_allocator)
			rl.DrawText(c_query, base_text_x + text_padding, text_y_i, font_size, COL_TEXT)
			// Bold effect
			rl.DrawText(c_query, base_text_x + text_padding + 1, text_y_i, font_size, COL_TEXT)

			// Ghost Text & Open Label
			user_text_width := rl.MeasureText(c_query, font_size)
			current_x := base_text_x + text_padding + user_text_width

			// Draw path completion if available
			if len(suggestion_suffix) > 0 {
				c_suffix := strings.clone_to_cstring(suggestion_suffix, context.temp_allocator)
				rl.DrawText(c_suffix, current_x, text_y_i, font_size, COL_GHOST)
				current_x += rl.MeasureText(c_suffix, font_size)
			}

			// Draw " --- Open" if applicable
			if len(suggestion_display_extra) > 0 {
				c_extra := strings.clone_to_cstring(
					suggestion_display_extra,
					context.temp_allocator,
				)
				rl.DrawText(c_extra, current_x, text_y_i, font_size, rl.Fade(COL_ACCENT, 0.7))
			}
		}
		// Blinking Cursor
		if (frame_counter / 30) % 2 == 0 {
			cursor_x := base_text_x
			if len(search_query) > 0 {
				c_query := strings.clone_to_cstring(query_str, context.temp_allocator)
				cursor_x += rl.MeasureText(c_query, font_size) + text_padding + 3
			}
			rl.DrawRectangle(cursor_x, cursor_y_i, 3, cursor_height, COL_ACCENT)
		}

		// 4. Results List
		result_y := i32(bar_y + bar_height + 10)
		icon_size := 24

		for res in current_results {
			// Placeholder for icon
			rl.DrawRectangle(i32(bar_x + 20), result_y, i32(icon_size), i32(icon_size), COL_ICON)

			// Text
			display_path := res
			if len(display_path) > 60 {
				// Proper truncation with "..."
				suffix := display_path[len(display_path) - 57:]
				display_path = fmt.tprintf("...%s", suffix)
			}

			c_path := strings.clone_to_cstring(display_path, context.temp_allocator)
			rl.DrawText(c_path, i32(bar_x) + 20 + i32(icon_size) + 10, result_y + 2, 20, COL_TEXT)

			result_y += 30
		}

		rl.EndDrawing()
	}
}
