#include "SearchEngine.h"
#include "raylib.h"
#include <string>
#include <vector>

int main() {
    /*
     * Initialization of the Window
     * ----------------------------
     */
    const int screenWidth = 700;
    const int screenHeight = 400; // Increased height to show results

    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TRANSPARENT | FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Raylib - Spotlight Search Phase 1");
    SetTargetFPS(60);

    SearchEngine engine;

    /*
     * Application State Variables
     * ---------------------------
     */
    std::string searchQuery = "";
    int frameCounter = 0; // To make the cursor blink

    /*
     * Tokyo Night Color Palette (Glassmorphism Edition)
     * -------------------------------------------------
     */
    const Color COL_BAR_BG = {20, 25, 40, 200};   // Deep Glass
    const Color COL_BORDER = {120, 140, 180, 80}; // Soft Rim
    const Color COL_TEXT = {192, 202, 245, 255};
    const Color COL_ACCENT = {122, 162, 247, 255};
    const Color COL_ICON = {169, 177, 214, 200};

    /*
     * Search Bar Geometry
     * -------------------
     */
    const float margin = 10;
    const float barWidth = screenWidth - (margin * 2);
    const float barHeight = 70 - (margin * 2); // Fixed height for input bar
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
        bool textChanged = false;
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 32) && (key <= 125)) {
                searchQuery += (char)key;
                textChanged = true;
            }
            key = GetCharPressed();
        }

        // Handle Backspace
        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            if (!searchQuery.empty()) {
                if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) {
                    while (!searchQuery.empty() && searchQuery.back() == ' ')
                        searchQuery.pop_back();
                    while (!searchQuery.empty() && searchQuery.back() != ' ')
                        searchQuery.pop_back();
                } else {
                    searchQuery.pop_back();
                }
                textChanged = true;
            }
        }

        // Trigger Search if text changed
        if (textChanged) {
            engine.Search(searchQuery);
        }

        frameCounter++;

        /*
         * Drawing Operations
         * ------------------
         */
        BeginDrawing();
        ClearBackground(BLANK);

        // 1. Base Glass Layer (Search Bar)
        DrawRectangleRounded(searchBarRect, 0.8f, 20, COL_BAR_BG);

        // 3. Rim / Border
        DrawRectangleRoundedLines(searchBarRect, 0.8f, 20, COL_BORDER);

        /*
         * Draw Magnifying Glass Icon
         * --------------------------
         */
        int iconCenterX = (int)barX + 25;
        int centerY = (int)(barY + barHeight / 2);
        int iconCenterY = centerY;
        float radius = 8.5f;

        DrawCircleLines(iconCenterX, iconCenterY, radius, COL_ICON);
        DrawCircleLines(iconCenterX, iconCenterY, radius - 0.5f, COL_ICON);

        Vector2 start = {(float)iconCenterX + 5, (float)iconCenterY + 5};
        Vector2 end = {(float)iconCenterX + 12, (float)iconCenterY + 12};
        DrawLineEx(start, end, 3.0f, COL_ICON);

        /*
         * Draw Text & Cursor
         * ------------------
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
            DrawText(searchQuery.c_str(), baseTextX + textPadding, textY, fontSize, COL_TEXT);
            DrawText(searchQuery.c_str(), baseTextX + textPadding + 1, textY, fontSize, COL_TEXT);
        }

        if ((frameCounter / 30) % 2 == 0) {
            int cursorX = baseTextX;
            if (!searchQuery.empty()) {
                cursorX += MeasureText(searchQuery.c_str(), fontSize) + textPadding + 3;
            }
            DrawRectangle(cursorX, cursorY, 3, cursorHeight, COL_ACCENT);
        }

        /*
         * Render Results (Prototype)
         * --------------------------
         */
        std::vector<std::string> results = engine.GetResults();
        int resultY = barY + barHeight + 10;

        for (const auto &res : results) {
            DrawText(res.c_str(), barX + 20, resultY, 20, COL_TEXT);
            resultY += 25;
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
