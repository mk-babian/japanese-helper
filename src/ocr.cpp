#include "include/ocr.h"
#include "include/get_exec_path.h"

std::filesystem::path path = get_executable_path().parent_path() / L"screenshot.png";

// Find the CLSID for a given image encoder (e.g. "image/png")
int get_encoder_clsid(const WCHAR* format, CLSID* pClsid){
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    Gdiplus::ImageCodecInfo* info = (Gdiplus::ImageCodecInfo*)malloc(size);
    if (!info) return -1;
    Gdiplus::GetImageEncoders(num, size, info);

    for (UINT i = 0; i < num; ++i){
        if (wcscmp(info[i].MimeType, format) == 0){
            *pClsid = info[i].Clsid;
            free(info);
            return i;
        }
    }
    free(info);
    return -1;
}

// Windows only
// Vibe coded all of this... I have no clue what's happening.
void capture_windows(void){
    int start_w = 0, start_h = 0;
    int cap_w   = 100, cap_h = 100;

    POINT p;
    if (GetCursorPos(&p)){
        cap_w = p.x;
        cap_h = p.y;
    }

    Gdiplus::GdiplusStartupInput gdiplus_startup_input;
    ULONG_PTR token;
    Gdiplus::GdiplusStartup(&token, &gdiplus_startup_input, nullptr);

    int width  = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HDC h_screen  = GetDC(nullptr);
    HDC h_mem     = CreateCompatibleDC(h_screen);
    HBITMAP h_bmp = CreateCompatibleBitmap(h_screen, cap_w, cap_h);
    HBITMAP h_old = (HBITMAP)SelectObject(h_mem, h_bmp);

    // Copy pixels from the screen into our bitmap
    BitBlt(h_mem, 0, 0, cap_w, cap_h, h_screen, start_w, start_h, SRCCOPY | CAPTUREBLT);
    SelectObject(h_mem, h_old);  // deselect before handing to GDI+

    // Wrap the HBITMAP and save it
    Gdiplus::Bitmap bitmap(h_bmp, nullptr);
    CLSID png_clsid;
    get_encoder_clsid(L"image/png", &png_clsid);
    bitmap.Save(path.c_str(), &png_clsid, nullptr);

    DeleteObject(h_bmp);
    DeleteDC(h_mem);
    ReleaseDC(nullptr, h_screen);
    Gdiplus::GdiplusShutdown(token);
}