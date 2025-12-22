#include "raylib.h"
#include <string>

int main() {
    /*
     * Initialization of the Window
     * ----------------------------
     * We set the dimensions to 640x80 to replicate a standard search bar size.
     * Flags are set to:
     * 1. FLAG_WINDOW_UNDECORATED: To remove the OS title bar and borders.
     * 2. FLAG_WINDOW_TRANSPARENT: To allow non-rectangular shapes (pill shape).
     * 3. FLAG_MSAA_4X_HINT: To enable Anti-Aliasing for smooth circle/line
     * rendering.
     */
    const int screenWidth = 700;
    const int screenHeight = 70;

    // Remove window decorations AND make background transparent
    // Enable 4x MSAA for smoother circles/lines
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TRANSPARENT | FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Raylib - Spotlight Search Phase 1");
    SetTargetFPS(60);

    /*
     * Application State Variables
     * ---------------------------
     * searchQuery: Stores the current text typed by the user.
     * frameCounter: Used to toggle the blinking cursor visibility.
     */
    std::string searchQuery = "";
    int frameCounter = 0; // To make the cursor blink

    /*
     * Tokyo Night Color Palette (Glassmorphism Edition)
     * -------------------------------------------------
     * COL_BAR_BG: Deep, high-transparency blue for the base glass.
     * COL_BORDER: Faint white/blue rim to define edges.
     * COL_HIGHLIGHT: Subtle white overlay for the top reflection.
     */
    const Color COL_BAR_BG = {20, 25, 40, 200};      // Deep Glass
    const Color COL_BORDER = {120, 140, 180, 80};    // Soft Rim
    const Color COL_TEXT = {192, 202, 245, 255};
    const Color COL_ACCENT = {122, 162, 247, 255};
    const Color COL_ICON = {169, 177, 214, 200};

    /*
     * Search Bar Geometry
     * -------------------
     */
    const float margin = 10; // Increased for border space
    const float barWidth = screenWidth - (margin * 2);
    const float barHeight = screenHeight - (margin * 2);
    const float barX = margin;
    const float barY = margin;

    Rectangle searchBarRect = {barX, barY, barWidth, barHeight};

    /*
     * Window Dragging State
     * ---------------------
     */
    bool isDragging = false;

    // Main game loop
    while (!WindowShouldClose()) {
        /*
         * Update Logic: Window Dragging
         * -----------------------------
         * We use GetMouseDelta() which returns the mouse movement in screen coordinates
         * (or relative to the previous frame). This is the standard, stable way to
         * handle dragging in Raylib for undecorated windows.
         */
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(GetMousePosition(), searchBarRect)) {
                isDragging = true;
            }
        }

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            isDragging = false;
        }

        if (isDragging) {
            Vector2 delta = GetMouseDelta();
            Vector2 winPos = GetWindowPosition();
            SetWindowPosition((int)(winPos.x + delta.x), (int)(winPos.y + delta.y));
        }
        /*
         * Update Logic: Text Input
         * ------------------------
         */
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 32) && (key <= 125)) {
                searchQuery += (char)key;
            }
            key = GetCharPressed();
        }

        // Handle Backspace
        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            if (!searchQuery.empty()) {
                // Check for Option/Alt key for word deletion
                if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) {
                    // Remove trailing spaces first if any
                    while (!searchQuery.empty() && searchQuery.back() == ' ') {
                        searchQuery.pop_back();
                    }
                    // Remove characters until a space is found
                    while (!searchQuery.empty() && searchQuery.back() != ' ') {
                        searchQuery.pop_back();
                    }
                } else {
                    // Standard single character deletion
                    searchQuery.pop_back();
                }
            }
        }

        frameCounter++;

        /*
         * Drawing Operations
         * ------------------
         */
        BeginDrawing();
        ClearBackground(BLANK);

        // 1. Base Glass Layer
        DrawRectangleRounded(searchBarRect, 0.8f, 20, COL_BAR_BG);

        // 2. [Removed Gloss Effect for uniform look]

        // 3. Rim / Border
        DrawRectangleRoundedLines(searchBarRect, 0.8f, 20, COL_BORDER);

        /*
         * Draw Magnifying Glass Icon
         * --------------------------
         * Constructed using geometric primitives (circles and lines) rather than an image texture.
         * Radius 10.0f is used for the larger search bar size.
         */
        int iconCenterX = (int)barX + 25;
        int centerY = (int)(barY + barHeight / 2);
        int iconCenterY = centerY;
        float radius = 8.5f; // Increased size

        // Outer and Inner circle (for thickness)
        DrawCircleLines(iconCenterX, iconCenterY, radius, COL_ICON);
        DrawCircleLines(iconCenterX, iconCenterY, radius - 0.5f,
                        COL_ICON); // Thicker line

        // The handle of the magnifying glass
        Vector2 start = {(float)iconCenterX + 5, (float)iconCenterY + 5};
        Vector2 end = {(float)iconCenterX + 12, (float)iconCenterY + 12};
        DrawLineEx(start, end, 3.0f, COL_ICON);

        /*
         * Draw Text & Cursor
         * ------------------
         * We calculate positions dynamically to center them vertically.
         * A +2 pixel offset is added ("Nudge") to visually align the text baseline
         * with the icon's center, as mathematical centering can look too high.
         */
        int baseTextX = (int)barX + 55;
        int textPadding = 4;
        int fontSize = 28;
        int cursorHeight = 32;

        int cursorY = (int)(barY + (barHeight - cursorHeight) / 2) + 1;
        int textY = (cursorY + (cursorHeight - fontSize) / 2) + 1;

        if (searchQuery.empty()) {
            DrawText("Max Search here", baseTextX + textPadding, textY, fontSize,
                     Fade(COL_TEXT, 0.5f));
        } else {
            // Actual Query Text
            // We draw the text twice with a 1px offset to simulate a "Bold" font weight.
            DrawText(searchQuery.c_str(), baseTextX + textPadding, textY, fontSize, COL_TEXT);
            DrawText(searchQuery.c_str(), baseTextX + textPadding + 1, textY, fontSize, COL_TEXT);
        }

        /*
         * Draw Blinking Cursor
         * --------------------
         * The cursor blinks every 30 frames (0.5 seconds at 60 FPS).
         * It moves to follow the text width.
         */
        if ((frameCounter / 30) % 2 == 0) {
            int cursorX = baseTextX;
            if (!searchQuery.empty()) {
                // Move cursor to end of text + padding
                cursorX += MeasureText(searchQuery.c_str(), fontSize) + textPadding +
                           3; // +3 for bold offset
            }
            DrawRectangle(cursorX, cursorY, 3, cursorHeight,
                          COL_ACCENT); // Thicker cursor
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
