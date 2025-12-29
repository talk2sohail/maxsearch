#include "MacUtils.h"
#include "SearchEngine.h"
#include "raylib.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

// Helper for case-insensitive prefix check
bool StartsWithCaseInsensitive(const std::string &full, const std::string &prefix) {
    if (prefix.length() > full.length())
        return false;
    return std::equal(prefix.begin(), prefix.end(), full.begin(),
                      [](char a, char b) { return std::tolower(a) == std::tolower(b); });
}

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

    // Icon Cache to avoid re-loading textures every frame
    std::map<std::string, Texture2D> iconCache;

    /*
     * Tokyo Night Color Palette (Glassmorphism Edition)
     * -------------------------------------------------
     */
    const Color COL_BAR_BG = {20, 25, 40, 200};   // Deep Glass
    const Color COL_BORDER = {120, 140, 180, 80}; // Soft Rim
    const Color COL_TEXT = {192, 202, 245, 255};
    const Color COL_ACCENT = {122, 162, 247, 255};
    const Color COL_ICON = {169, 177, 214, 200};
    const Color COL_GHOST = {169, 177, 214, 100}; // Dimmed text for suggestions

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
         * Fetch Results & Calculate Suggestion
         * ------------------------------------
         */
        std::vector<std::string> results = engine.GetResults();
        std::string suggestionSuffix = "";
        std::string suggestedFilename = "";

        if (!searchQuery.empty() && !results.empty()) {
            // Get just the filename from the full path
            std::filesystem::path firstPath(results[0]);
            suggestedFilename = firstPath.filename().string();

            if (StartsWithCaseInsensitive(suggestedFilename, searchQuery)) {
                // The part of the filename we haven't typed yet
                suggestionSuffix = suggestedFilename.substr(searchQuery.length());
            }
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

        // Handle Tab Completion
        if (IsKeyPressed(KEY_TAB) && !suggestionSuffix.empty()) {
            searchQuery += suggestionSuffix;
            textChanged = true;
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
         * Draw Text, Suggestion & Cursor
         * ------------------------------
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
            // Draw User Text
            DrawText(searchQuery.c_str(), baseTextX + textPadding, textY, fontSize, COL_TEXT);
            // Bold effect
            DrawText(searchQuery.c_str(), baseTextX + textPadding + 1, textY, fontSize, COL_TEXT);

            // Draw Ghost Text (Suggestion)
            if (!suggestionSuffix.empty()) {
                int userTextWidth = MeasureText(searchQuery.c_str(), fontSize);
                DrawText(suggestionSuffix.c_str(), baseTextX + textPadding + userTextWidth, textY,
                         fontSize, COL_GHOST);
            }
        }

        if ((frameCounter / 30) % 2 == 0) {
            int cursorX = baseTextX;
            if (!searchQuery.empty()) {
                cursorX += MeasureText(searchQuery.c_str(), fontSize) + textPadding + 3;
            }
            DrawRectangle(cursorX, cursorY, 3, cursorHeight, COL_ACCENT);
        }

        /*
         * Render Results with Icons
         * -------------------------
         */
        int resultY = barY + barHeight + 10;
        int iconSize = 24;

        for (const auto &res : results) {
            // 1. Icon Management
            if (iconCache.find(res) == iconCache.end()) {
                // Not in cache, load it (32x32 for high DPI crispness)
                Image img = LoadMacOSIcon(res, 32);
                Texture2D tex = LoadTextureFromImage(img);
                UnloadImage(img); // Free raw CPU data
                iconCache[res] = tex;
            }

            Texture2D icon = iconCache[res];
            DrawTexturePro(icon, {0, 0, (float)icon.width, (float)icon.height},
                           {(float)(barX + 20), (float)resultY, (float)iconSize, (float)iconSize},
                           {0, 0}, 0.0f, WHITE);

            // 2. Text Display
            std::string displayPath = res;
            if (displayPath.length() > 60) {
                displayPath = "..." + displayPath.substr(displayPath.length() - 57);
            }

            // Offset text to right of icon
            DrawText(displayPath.c_str(), barX + 20 + iconSize + 10, resultY + 2, 20, COL_TEXT);
            resultY += 30; // Increased spacing
        }

        EndDrawing();
    }

    // Cleanup Textures
    for (auto &entry : iconCache) {
        UnloadTexture(entry.second);
    }

    CloseWindow();
    return 0;
}