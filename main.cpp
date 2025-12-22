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
     * Tokyo Night Color Palette
     * -------------------------
     * COL_BAR_BG: Semi-transparent dark blue for the glassy effect.
     * COL_TEXT: Soft white for readability.
     * COL_ACCENT: Bright blue for the cursor.
     * COL_ICON: Dimmed white for the magnifying glass.
     */
    const Color COL_BAR_BG = {36, 40, 59, 230};
    const Color COL_TEXT = {169, 177, 214, 255};
    const Color COL_ACCENT = {122, 162, 247, 255};
    const Color COL_ICON = {169, 177, 214, 200};

    /*
     * Search Bar Geometry
     * -------------------
     * We calculate the bar dimensions based on the screen size minus a small
     * margin. This margin allows the rounded corners to be anti-aliased correctly
     * against the transparent background.
     */
    const float margin = 5; // Reduced padding
    const float barWidth = screenWidth - (margin * 2);
    const float barHeight = screenHeight - (margin * 2);
    const float barX = margin;
    const float barY = margin;

    Rectangle searchBarRect = {barX, barY, barWidth, barHeight};

    /*
     * Window Dragging State
     * ---------------------
     * Since the window is undecorated, we must manually handle window movement.
     * dragStartOffset stores the mouse position relative to the window when
     * dragging starts.
     */
    bool isDragging = false;
    Vector2 dragStartOffset = {0, 0};

    // Main game loop
    while (!WindowShouldClose()) {
        /*
         * Update Logic: Window Dragging
         * -----------------------------
         * 1. Detect Left Click: Start dragging if clicked inside the bar.
         * 2. Detect Release: Stop dragging.
         * 3. While Dragging: Calculate the delta (movement) and update the window
         * position.
         */
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(GetMousePosition(), searchBarRect)) {
                isDragging = true;
                dragStartOffset = GetMousePosition();
            }
        }

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            isDragging = false;
        }

        if (isDragging) {
            Vector2 mousePos = GetMousePosition();
            Vector2 delta = {mousePos.x - dragStartOffset.x, mousePos.y - dragStartOffset.y};

            // Get current window pos to update it
            Vector2 winPos = GetWindowPosition();
            SetWindowPosition((int)(winPos.x + delta.x), (int)(winPos.y + delta.y));
        }

        /*
         * Update Logic: Text Input
         * ------------------------
         * We capture character presses for typing and KEY_BACKSPACE for deletion.
         * GetCharPressed() handles layout-independent text input (e.g., Shift+Key).
         */
        int key = GetCharPressed();
        while (key > 0) {
            // Only accept visible characters (ASCII 32..125)
            if ((key >= 32) && (key <= 125)) {
                searchQuery += (char)key;
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            if (!searchQuery.empty()) {
                searchQuery.pop_back();
            }
        }

        frameCounter++;

        /*
         * Drawing Operations
         * ------------------
         */
        BeginDrawing();
        /*
         * clearBackground(BLANK) is crucial for the transparent window effect.
         * It clears the buffer to fully transparent pixels.
         */
        ClearBackground(BLANK);

        /*
         * Draw Search Bar Background
         * --------------------------
         * We use a high roundness value (0.5f) to create a pill-shaped container.
         */
        DrawRectangleRounded(searchBarRect, 0.7f, 10, COL_BAR_BG);

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
