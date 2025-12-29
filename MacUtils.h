#ifndef MAC_UTILS_H
#define MAC_UTILS_H

#include "raylib.h"
#include <string>

// Loads the system icon for a given file path and returns it as a Raylib Image.
// The caller is responsible for Unloading the image (UnloadImage) or converting it to a Texture.
Image LoadMacOSIcon(const std::string& path, int size);

#endif
