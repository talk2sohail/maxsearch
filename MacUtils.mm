#include "MacUtils.h"
#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>

Image LoadMacOSIcon(const std::string& path, int size) {
    @autoreleasepool {
        NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
        NSImage* icon = [[NSWorkspace sharedWorkspace] iconForFile:nsPath];

        if (!icon) {
            return GenImageColor(size, size, BLANK);
        }

        // Resize the image to the desired dimensions
        NSSize newSize = NSMakeSize(size, size);
        [icon setSize:newSize];

        // Create a bitmap representation
        // We use a specific method to ensure we get a bitmap suitable for raw data extraction
        CGImageRef cgImage = [icon CGImageForProposedRect:NULL context:NULL hints:NULL];
        if (!cgImage) {
            return GenImageColor(size, size, BLANK);
        }

        // Create a context to draw the image into to get raw RGBA data
        // Raylib expects RGBA (4 bytes per pixel)
        size_t bitsPerComponent = 8;
        size_t bytesPerPixel = 4;
        size_t bytesPerRow = size * bytesPerPixel;
        size_t dataSize = size * bytesPerRow;

        unsigned char* rawData = (unsigned char*)calloc(1, dataSize);
        if (!rawData) {
             return GenImageColor(size, size, BLANK);
        }

        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(rawData,
                                                     size,
                                                     size,
                                                     bitsPerComponent,
                                                     bytesPerRow,
                                                     colorSpace,
                                                     kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);

        if (!context) {
            free(rawData);
            CGColorSpaceRelease(colorSpace);
            return GenImageColor(size, size, BLANK);
        }

        // Draw the image into the context
        CGRect rect = CGRectMake(0, 0, size, size);
        CGContextDrawImage(context, rect, cgImage);

        // Cleanup CoreGraphics objects
        CGContextRelease(context);
        CGColorSpaceRelease(colorSpace);

        // Construct Raylib Image
        Image rayImage;
        rayImage.data = rawData; // Raylib takes ownership of this pointer
        rayImage.width = size;
        rayImage.height = size;
        rayImage.mipmaps = 1;
        rayImage.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

        return rayImage;
    }
}
