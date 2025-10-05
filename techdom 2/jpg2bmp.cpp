#include <windows.h>
#include <gdiplus.h>
#include <iostream>

#pragma comment (lib, "Gdiplus.lib")

using namespace Gdiplus;

// Initialize GDI+ once in your program
class GDIPlusManager {
public:
    GDIPlusManager() {
        GdiplusStartupInput gdiplusStartupInput;
        GdiplusStartup(&token, &gdiplusStartupInput, NULL);
    }

    ~GDIPlusManager() {
        GdiplusShutdown(token);
    }

private:
    ULONG_PTR token;
};

int convertJPGtoBMP(const wchar_t* inputPath, const wchar_t* outputPath) {
    GDIPlusManager gdiManager;  // Initializes GDI+

    // Load the JPEG image
    Image image(inputPath);

    if (image.GetLastStatus() != Ok) {
        std::wcout << L"Failed to load image: " << inputPath << std::endl;
        return -1;
    }

    // Save as BMP
    CLSID bmpClsid;
    // Get the encoder CLSID for BMP format
    UINT num, size;
    GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL) return -1;

    GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(pImageCodecInfo[i].MimeType, L"image/bmp") == 0) {
            bmpClsid = pImageCodecInfo[i].Clsid;
            break;
        }
    }
    free(pImageCodecInfo);

    if (image.Save(outputPath, &bmpClsid, NULL) != Ok) {
        std::wcout << L"Failed to save image as BMP." << std::endl;
        return -1;
    }

    std::wcout << L"Image converted to BMP successfully!" << std::endl;
    return 0;
}
