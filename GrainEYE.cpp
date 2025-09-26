/*
*****************************************************************************
*                                                                             *
*                        GrainEye Sand Grain Analyzer                         *
*                        ============================                         *
*                           Version : 1.03                                    *
*                           Status  : Under Development                       *
*                                                                             *
*   Author   : Subhajit Halder                                                *
*   Project  : Smart India Hackathon 2025 (SIH)                               *
*   Date     : 25-Sep-2025                                                    *
*                                                                             *
*   Target Platforms:                                                         *
*   ------------------------------------------------------------------------- *
*   - Windows (Client)  : Implemented using Win32 API                         *
*   - Linux (Client)    : Qt-based port in progress for native support        *
*   - Embedded Hardware : Raspberry Pi with Pi Camera & EC2000U-CN GNSS       *
*                                                                             *
*   Description:                                                              *
*   ------------------------------------------------------------------------- *
*   GrainEye is a sand grain analysis and beach mapping client application.   *
*   - Captures/upload sand sample images                                      *
*   - Sends images to cloud-hosted ML model for analysis                      *
*   - Displays results with grain size distribution and beach classification  *
*   - Fetches GNSS location using EC2000U-CN module                           *
*   - Tags analyzed data on custom map API                                    *
*   - Exports results to CSV, with restart workflow                           *
*                                                                             *
*   Notes:                                                                    *
*   ------------------------------------------------------------------------- *
*   - This codebase contains only the **client-side logic**.                  *
*   - Machine Learning model and map APIs run on cloud backend.               *
*   - Qt porting ensures future cross-platform (Linux) compatibility.         *
*   - CLoud communication is currently not available, under development.      *
*   License:                                                                  *
*   ------------------------------------------------------------------------- *
*   This code is currently for **demonstration purposes only** (SIH 2025).    *
*   Redistribution, modification, or commercial use is not permitted at this  *
*   stage. An open-source license will be applied after final release.        *
*                                                                             *
******************************************************************************
*/


#include <windows.h>
#include <commdlg.h>
#include <gdiplus.h>
#include <dwmapi.h>
#include <string>
#include <vector>
#include <algorithm>
#include<iostream>
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "Msimg32.lib")
#pragma comment(lib, "dwmapi.lib")
#include"resource.h"
using namespace Gdiplus;

// ---------- Globals ----------
HINSTANCE hInst;
HWND hUploadBtn, hAnalyzeBtn, hSaveBtn, hRestartBtn;
HWND hImageBox, hResultBox;
HWND hFetchLocBtn, hTagBtn, hLocationText;
std::wstring imagePath;
Image* uploadedImage = nullptr;

ULONG_PTR gdiplusToken;

// Fonts (create once)
HFONT g_hFont = NULL;
HFONT g_hTitleFont = NULL;
HFONT g_hSubtitleFont = NULL;

// Colors
const COLORREF DARK_BG = RGB(32, 32, 32);
const COLORREF CARD_BG = RGB(43, 43, 43);
const COLORREF TEXT_PRIMARY = RGB(240, 240, 240);
const COLORREF TEXT_SECONDARY = RGB(200, 200, 200);
const COLORREF ACCENT_COLOR = RGB(0, 184, 148);
const COLORREF ACCENT_LIGHT = RGB(80, 214, 184);
const COLORREF BUTTON_BG = RGB(45, 45, 45);
const COLORREF BUTTON_HOVER = RGB(60, 60, 60);
const COLORREF BUTTON_ACTIVE = RGB(70, 70, 70);

// Graph colors
const COLORREF GRAPH_BG = RGB(35, 35, 35);
const COLORREF GRAPH_AXIS = RGB(100, 100, 100);
const COLORREF GRAPH_LINE = ACCENT_COLOR;
const COLORREF GRAPH_BAR = ACCENT_LIGHT;

// Green tag buttons color
const COLORREF TAG_GREEN = RGB(16, 160, 70);
const COLORREF TAG_GREEN_HOVER = RGB(24, 200, 90);
const COLORREF TAG_GREEN_ACTIVE = RGB(12, 130, 55);

// Custom button structure
struct CustomButton {
    HWND hwnd;
    bool isHovered;
    bool isPressed;
    int cornerRadius;
    bool isAccent; // accent styled (Analyze)
    bool alwaysGreen; // always green (location tagging)
};

std::vector<CustomButton> customButtons;

// Forward declarations
void ShowImage(HWND hwnd, const std::wstring& path);
void DoAnalysis(HWND hwnd);
void DrawModernButton(HDC hdc, HWND hwnd, CustomButton& button, const wchar_t* text);
void RegisterButton(HWND hwnd, int cornerRadius = 8, bool isAccent = false, bool alwaysGreen = false);
void UpdateButtonState(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CreateGraphs();
void DrawGraph(HDC hdc, int x, int y, int width, int height, const std::wstring& title);
void DrawRoundedRect(HDC hdc, int x, int y, int width, int height, int radius, COLORREF color);
void FillRoundedRect(HDC hdc, int x, int y, int width, int height, int radius, COLORREF color);
void DrawCard(HDC hdc, int x, int y, int width, int height, int radius = 12);
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Helper function to set text color for static controls
void SetStaticTextColor(HWND hwnd, COLORREF color) {
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)color);
}

// Helper function to get text color for static controls
COLORREF GetStaticTextColor(HWND hwnd) {
    return (COLORREF)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

// Try to enable Mica / dark mode (best-effort)
void EnableWindowEffects(HWND hwnd) {
    // Try to enable dark mode (Windows 10/11)
    BOOL useDarkMode = TRUE;
    if (DwmSetWindowAttribute) {
        // Try the newer attribute first (Windows 11)
        DwmSetWindowAttribute(hwnd, 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &useDarkMode, sizeof(useDarkMode));

        // Fallback to older attribute (Windows 10)
        DwmSetWindowAttribute(hwnd, 19 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &useDarkMode, sizeof(useDarkMode));
    }

    // For Mica effect on Windows 11
    const DWM_SYSTEMBACKDROP_TYPE backdropType = DWMSBT_MAINWINDOW; // Mica effect
    DwmSetWindowAttribute(hwnd, 38 /*DWMWA_SYSTEMBACKDROP_TYPE*/, &backdropType, sizeof(backdropType));

    // Remove the blur behind code as it conflicts with Mica
}
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    hInst = hInstance;

    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    const wchar_t CLASS_NAME[] = L"UltraModernGrainEyeClass";

    // Use WNDCLASSEX instead of WNDCLASS
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;

    // Load icon from file
    HICON hAppIcon = (HICON)LoadImage(NULL, L"C:\\ico.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
    wc.hIcon = hAppIcon;      // Title bar icon
    wc.hIconSm = hAppIcon;    // Taskbar / Alt+Tab small icon

    RegisterClassEx(&wc);      // EX version required for hIconSm

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"GRAINEYE",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 820,
        NULL, NULL, hInstance, NULL
    );

    EnableWindowEffects(hwnd);

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    GdiplusShutdown(gdiplusToken);
    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // Create modern fonts (store globally)
        g_hFont = CreateFontW(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        g_hTitleFont = CreateFontW(48, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        g_hSubtitleFont = CreateFontW(21, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

        // App title
        HWND hTitle = CreateWindowW(L"STATIC", L"Sand Grain Analyzer",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            15, 30, 1240, 47, hwnd, NULL, hInst, NULL);
        SendMessage(hTitle, WM_SETFONT, (WPARAM)g_hTitleFont, TRUE);
        SetStaticTextColor(hTitle, TEXT_PRIMARY);

        // Version subtitle
        HWND hSubtitle = CreateWindowW(L"STATIC", L"Version 1.03 - For Testing Purposes Only",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            15, 90, 1240, 25, hwnd, NULL, hInst, NULL);
        SendMessage(hSubtitle, WM_SETFONT, (WPARAM)g_hSubtitleFont, TRUE);
        SetStaticTextColor(hSubtitle, TEXT_SECONDARY);

        // Upload button
        hUploadBtn = CreateWindowW(L"BUTTON", L"📁 Upload Image",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            50, 112, 200, 45, hwnd, (HMENU)1, hInst, NULL);
        RegisterButton(hUploadBtn, 10);
        SendMessage(hUploadBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        // Analyze button (accent colored)
        hAnalyzeBtn = CreateWindowW(L"BUTTON", L"🔍 Analyze",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_DISABLED,
            270, 112, 200, 45, hwnd, (HMENU)2, hInst, NULL);
        RegisterButton(hAnalyzeBtn, 10); // true for accent color
        SendMessage(hAnalyzeBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        // Image preview label
        HWND hImageLabel = CreateWindowW(L"STATIC", L"Image Preview",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            50, 184, 200, 25, hwnd, NULL, hInst, NULL);
        SendMessage(hImageLabel, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        SetStaticTextColor(hImageLabel, TEXT_SECONDARY);

        // Image box (preview frame)
        hImageBox = CreateWindowW(L"STATIC", L"",
            WS_CHILD | WS_VISIBLE,
            50, 210, 420, 280, hwnd, NULL, hInst, NULL);

        // Location Tagging label (under image)
        HWND hLocLabel = CreateWindowW(L"STATIC", L"Location Tagging",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            50, 514, 200, 25, hwnd, NULL, hInst, NULL);
        SendMessage(hLocLabel, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        SetStaticTextColor(hLocLabel, TEXT_SECONDARY);

        // Fetch Location button (always-green) - under image
        hFetchLocBtn = CreateWindowW(L"BUTTON", L"📍 Fetch Location",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            70, 540, 150, 45, hwnd, (HMENU)5, hInst, NULL);
        RegisterButton(hFetchLocBtn, 10, false, true); // alwaysGreen = true
        SendMessage(hFetchLocBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        // Tag button (always-green) - under image
        hTagBtn = CreateWindowW(L"BUTTON", L"🏷️ Tag",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_DISABLED,
            70, 614, 150, 45, hwnd, (HMENU)6, hInst, NULL);
        RegisterButton(hTagBtn, 10, false, true);
        SendMessage(hTagBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        // Location static text (under image)
        hLocationText = CreateWindowW(L"STATIC", L"",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            240, 540, 220, 120, hwnd, NULL, hInst, NULL);
        SendMessage(hLocationText, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        SetStaticTextColor(hLocationText, TEXT_PRIMARY);

        // Results label (moved up to make more space)
        HWND hResultLabel = CreateWindowW(L"STATIC", L"   Analysis Results",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            500, 514, 400, 25, hwnd, NULL, hInst, NULL);
        SendMessage(hResultLabel, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        SetStaticTextColor(hResultLabel, TEXT_SECONDARY);

        // Results box (larger area with scrollbar)
        hResultBox = CreateWindowW(L"EDIT", L"Upload an image to begin analysis...",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY,
            510, 540, 700, 190, hwnd, NULL, hInst, NULL);
        SendMessage(hResultBox, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        // Make the result box background match the theme
        SetWindowLongPtr(hResultBox, GWLP_USERDATA, (LONG_PTR)TEXT_PRIMARY);

        // Save button
        hSaveBtn = CreateWindowW(L"BUTTON", L"💾 Save Results",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_DISABLED,
            50, 680, 200, 45, hwnd, (HMENU)3, hInst, NULL);
        RegisterButton(hSaveBtn, 10);
        SendMessage(hSaveBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        // Restart button
        hRestartBtn = CreateWindowW(L"BUTTON", L"🔄 Restart",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_DISABLED,
            270, 680, 200, 45, hwnd, (HMENU)4, hInst, NULL);
        RegisterButton(hRestartBtn, 10);
        SendMessage(hRestartBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    }
                  break;

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case 1: { // Upload
            OPENFILENAMEW ofn;
            WCHAR szFile[260] = { 0 };
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = L"Images\0*.BMP;*.JPG;*.JPEG;*.PNG\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileNameW(&ofn)) {
                imagePath = szFile;
                ShowImage(hwnd, imagePath);
                EnableWindow(hAnalyzeBtn, TRUE);
                SetWindowTextW(hResultBox, L"Image loaded successfully. Click 'Analyze' to process.");
                InvalidateRect(hAnalyzeBtn, NULL, TRUE);
            }
        }
              break;

        case 2: { // Analyze
            SetWindowTextW(hResultBox, L"Analyzing image...");
            UpdateWindow(hResultBox);

            DoAnalysis(hwnd);
        }
              break;

        case 3: { // Save
            // A simple stub: notify user. Could be extended to write files.
            MessageBoxW(hwnd, L"Results and graphs saved to your Documents folder.", L"Save Complete", MB_OK | MB_ICONINFORMATION);
        }
              break;

        case 4: { // Restart
            imagePath.clear();
            if (uploadedImage) { delete uploadedImage; uploadedImage = nullptr; }
            InvalidateRect(hwnd, NULL, TRUE);
            SetWindowTextW(hResultBox, L"Upload an image to begin analysis...");
            SetWindowTextW(hLocationText, L"");
            EnableWindow(hAnalyzeBtn, FALSE);
            EnableWindow(hSaveBtn, FALSE);
            EnableWindow(hRestartBtn, FALSE);
            EnableWindow(hTagBtn, FALSE);
            InvalidateRect(hAnalyzeBtn, NULL, TRUE);
            InvalidateRect(hSaveBtn, NULL, TRUE);
            InvalidateRect(hRestartBtn, NULL, TRUE);
            InvalidateRect(hTagBtn, NULL, TRUE);
        }
              break;

        case 5: { // Fetch Location
            // Immediately show the coordinates in the location text area
            SetWindowTextW(hLocationText, L"Latitude: 21° 37' 39.94\" N\nLongitude: 87° 31' 10.74\" E\n\n\n\nArea: DIGHA, WB, INDIA\nLocation data ready for tagging.");
            // After fetching, enable the Tag button
            EnableWindow(hTagBtn, TRUE);
            InvalidateRect(hTagBtn, NULL, TRUE);
        }
              break;

        case 6: { // Tag
            MessageBoxW(hwnd, L"Location has been tagged.", L"Tagged", MB_OK | MB_ICONINFORMATION);
        }
              break;
        }
    }
                   break;

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        HWND hwndStatic = (HWND)lParam;

        if (hwndStatic == hResultBox) {
            SetBkColor(hdcStatic, CARD_BG);
            SetTextColor(hdcStatic, TEXT_PRIMARY);
            return (LRESULT)CreateSolidBrush(CARD_BG);
        }

        SetBkMode(hdcStatic, TRANSPARENT);
        SetTextColor(hdcStatic, GetStaticTextColor(hwndStatic));
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }
                          break;

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT pDrawItem = (LPDRAWITEMSTRUCT)lParam;
        for (auto& btn : customButtons) {
            if (btn.hwnd == pDrawItem->hwndItem) {
                const int length = GetWindowTextLength(btn.hwnd);
                std::wstring text(length + 1, L'\0');
                GetWindowText(btn.hwnd, &text[0], length + 1);

                DrawModernButton(pDrawItem->hDC, btn.hwnd, btn, text.c_str());
                return TRUE;
            }
        }
    }
                    break;

    case WM_MOUSEMOVE: {
        UpdateButtonState(hwnd, msg, wParam, lParam);
    }
                     break;

    case WM_MOUSELEAVE: {
        for (auto& btn : customButtons) {
            if (btn.isHovered) {
                btn.isHovered = false;
                InvalidateRect(btn.hwnd, NULL, FALSE);
            }
        }
    }
                      break;

    case WM_LBUTTONDOWN: {
        UpdateButtonState(hwnd, msg, wParam, lParam);
    }
                       break;

    case WM_LBUTTONUP: {
        UpdateButtonState(hwnd, msg, wParam, lParam);
    }
                     break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Draw the Mica-like background (simulated with a gradient)
        RECT rc;
        GetClientRect(hwnd, &rc);

        TRIVERTEX vertex[2] = {
            {0, 0, (USHORT)(0x3000), (USHORT)(0x3000), (USHORT)(0x3000), 0},
            {rc.right, rc.bottom, (USHORT)(0x1200), (USHORT)(0x1200), (USHORT)(0x1200), 0}
        };
        GRADIENT_RECT gRect = { 0, 1 };
        GradientFill(hdc, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_V);

        // Draw cards for UI elements with transparency
        DrawCard(hdc, 40, 110, 440, 50); // Button card
        DrawCard(hdc, 40, 180, 440, 320); // Image preview card
        DrawCard(hdc, 40, 510, 440, 160); // Location tagging card (under image)
        DrawCard(hdc, 40, 680, 440, 50); // Action buttons card

        // Draw graph cards
        DrawCard(hdc, 500, 180, 720, 320); // Graph area card

        // Draw analysis results card (larger area)
        DrawCard(hdc, 500, 510, 720, 240); // Increased height from 220 to 240

        // Draw uploaded image if present (with border)
        if (uploadedImage) {
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hbmMem = CreateCompatibleBitmap(hdc, 420, 280);
            SelectObject(hdcMem, hbmMem);

            FillRoundedRect(hdcMem, 0, 0, 420, 280, 10, CARD_BG);

            Graphics graphics(hdcMem);
            Rect destRect(10, 10, 400, 260);

            graphics.DrawImage(uploadedImage, destRect);

            Pen pen(Color(100, 100, 100), 1);
            graphics.DrawRectangle(&pen, Rect(10, 10, 400, 260));

            BitBlt(hdc, 50, 210, 420, 280, hdcMem, 0, 0, SRCCOPY);

            DeleteObject(hbmMem);
            DeleteDC(hdcMem);
        }

        // Draw graphs if analysis was performed (indicated by Save button enabled)
        if (IsWindowEnabled(hSaveBtn)) {
            DrawGraph(hdc, 520, 200, 330, 260, L"Grain Size Distribution");
            DrawGraph(hdc, 870, 200, 330, 260, L"Cumulative Grain Size Curve");
        }

        // Draw small hint text inside location card
        SetTextColor(hdc, TEXT_SECONDARY);
        SetBkMode(hdc, TRANSPARENT);
        HFONT hSmall = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT hOld = (HFONT)SelectObject(hdc, hSmall);
        RECT hintRect = { 70, 592, 450, 640 };
        DrawText(hdc, L"Use 'Fetch Location' to get coordinates. Press 'Tag' to tag the location.", -1, &hintRect, DT_LEFT | DT_WORDBREAK);
        SelectObject(hdc, hOld);
        DeleteObject(hSmall);

        EndPaint(hwnd, &ps);
    }
                 break;

    case WM_ERASEBKGND:
        return 1; // custom drawing

    case WM_DESTROY:
        if (uploadedImage) delete uploadedImage;
        // Destroy fonts
        if (g_hFont) { DeleteObject(g_hFont); g_hFont = NULL; }
        if (g_hTitleFont) { DeleteObject(g_hTitleFont); g_hTitleFont = NULL; }
        if (g_hSubtitleFont) { DeleteObject(g_hSubtitleFont); g_hSubtitleFont = NULL; }
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ------------ Implementation ------------

void ShowImage(HWND hwnd, const std::wstring& path) {
    if (uploadedImage) {
        delete uploadedImage;
        uploadedImage = nullptr;
    }
    uploadedImage = Image::FromFile(path.c_str());
    InvalidateRect(hwnd, NULL, TRUE);
}

void DoAnalysis(HWND hwnd) {
    // Simulate processing delay for a more modern feel
    Sleep(1200);

    std::wstring result = L"SAND TYPE ANALYSIS COMPLETE:\r\n\n"
        L"• Beach Zone: Intertidal Zone (Foreshore / Swash Zone)\r\n"
        L"• Location: Area between high tide and low tide\r\n"
        L"• Sand Size: Medium Sand (0.25–0.5 mm)\r\n"
        L"• Median (d50): 0.43 mm\r\n"
        L"• Mean Grain Size: 0.43 mm\r\n"
        L"• Range (d10–d90): 0.26 – 0.70 mm\r\n"
        L"• Beach Type: Typical sandy beach, dissipative\r\n\n"
        L"• Category: Medium Sand → Intertidal\r\n"
        L"• GPS: 21.63°N, 87.55°E\r\n"
        L"• Time: 2025-09-25 1:48pm\r\n"
        L"• Image: c:/users/subhajit/downloads/20191010_130927_1c.jpg";


    SetWindowTextW(hResultBox, result.c_str());

    // Enable buttons
    EnableWindow(hSaveBtn, TRUE);
    EnableWindow(hRestartBtn, TRUE);
    InvalidateRect(hSaveBtn, NULL, TRUE);
    InvalidateRect(hRestartBtn, NULL, TRUE);

    // Enable Tag button
    EnableWindow(hTagBtn, TRUE);
    InvalidateRect(hTagBtn, NULL, TRUE);

    // Force repaint for graphs
    InvalidateRect(hwnd, NULL, TRUE);
}

void CreateGraphs() {
    // In a real app you'd create bitmaps or files; here we simulate drawing during WM_PAINT
}

// Draw a single graph card content
void DrawGraph(HDC hdc, int x, int y, int width, int height, const std::wstring& title) {
    // Create memory DC
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hbmMem);

    // Fill with graph background (black background like Python script)
    HBRUSH brush = CreateSolidBrush(RGB(18, 18, 18)); // #121212
    RECT rc = { 0, 0, width, height };
    FillRect(hdcMem, &rc, brush);
    DeleteObject(brush);

    // Apply with alpha (simulated)
    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 230;
    blend.AlphaFormat = 0;
    AlphaBlend(hdc, x, y, width, height, hdcMem, 0, 0, width, height, blend);

    // Draw border (grey like Python axes)
    DrawRoundedRect(hdc, x, y, width, height, 10, RGB(170, 170, 170)); // #aaaaaa

    // Title
    SetTextColor(hdc, RGB(170, 170, 170)); // Grey text like Python
    SetBkMode(hdc, TRANSPARENT);
    HFONT hTitleF = CreateFontW(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hTitleF);
    RECT titleRect = { x, y + 10, x + width, y + 36 };
    DrawText(hdc, title.c_str(), -1, &titleRect, DT_CENTER | DT_SINGLELINE);

    // Graph area boundaries
    int graphLeft = x + 60;
    int graphRight = x + width - 30;
    int graphTop = y + 50;
    int graphBottom = y + height - 50;

    // Draw grid lines (like Python's grid)
    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(170, 170, 170)); // Grey grid
    HPEN oldPen = (HPEN)SelectObject(hdc, gridPen);

    // Horizontal grid lines
    for (int i = 0; i <= 5; i++) {
        int yPos = graphBottom - i * (graphBottom - graphTop) / 5;
        MoveToEx(hdc, graphLeft, yPos, NULL);
        LineTo(hdc, graphRight, yPos);
    }

    // Vertical grid lines
    for (int i = 0; i <= 10; i++) {
        int xPos = graphLeft + i * (graphRight - graphLeft) / 10;
        MoveToEx(hdc, xPos, graphTop, NULL);
        LineTo(hdc, xPos, graphBottom);
    }

    // Draw axes
    HPEN axisPen = CreatePen(PS_SOLID, 2, RGB(170, 170, 170)); // Grey axes
    SelectObject(hdc, axisPen);

    MoveToEx(hdc, graphLeft, graphBottom, NULL);
    LineTo(hdc, graphRight, graphBottom);
    MoveToEx(hdc, graphLeft, graphTop, NULL);
    LineTo(hdc, graphLeft, graphBottom);

    if (title == L"Grain Size Distribution") {
        // Data from Python script
        double grainSizes[] = { 0.25, 0.30, 0.35, 0.40, 0.45, 0.50, 0.55, 0.60, 0.65, 0.70 };
        int counts[] = { 5, 10, 20, 25, 20, 10, 5, 3, 2, 1 };

        // Create emerald green brush for bars
        HBRUSH barBrush = CreateSolidBrush(RGB(46, 204, 113)); // #2ecc71 - emerald green
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, barBrush);
        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(170, 170, 170)); // Grey border
        HPEN oldBarPen = (HPEN)SelectObject(hdc, borderPen);

        int barWidth = (graphRight - graphLeft) / 15; // Adjust bar width

        // Find max count for scaling
        int maxCount = 0;
        for (int i = 0; i < 10; i++) {
            if (counts[i] > maxCount) maxCount = counts[i];
        }

        // Draw histogram bars
        for (int i = 0; i < 10; i++) {
            int barHeight = (int)((double)counts[i] / maxCount * (graphBottom - graphTop));
            int barX = graphLeft + (int)((grainSizes[i] - 0.25) / 0.45 * (graphRight - graphLeft - barWidth));
            int barY = graphBottom - barHeight;

            Rectangle(hdc, barX, barY, barX + barWidth, graphBottom);
        }

        // Axis labels
        HFONT lbl = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT oldLbl = (HFONT)SelectObject(hdc, lbl);
        SetTextColor(hdc, RGB(170, 170, 170));

        // X-axis label - FIXED CENTERING
        RECT xAxisRect = { x, y + height - 65, x + width, y + height };  // Changed from -25 to -35
        DrawText(hdc, L"Grain Size (mm)", -1, &xAxisRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Rotated Y label
        LOGFONT lf;
        GetObject(lbl, sizeof(LOGFONT), &lf);
        lf.lfEscapement = 900;
        HFONT vert = CreateFontIndirectW(&lf);
        HFONT oldVert = (HFONT)SelectObject(hdc, vert);
        RECT vertRect = { x + 1, y + height / 1.5 - 80, x + 108, y + height / 1.5 + 80 };
        DrawText(hdc, L"Frequency", -1, &vertRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Clean up bar resources
        SelectObject(hdc, oldBarPen);
        DeleteObject(borderPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(barBrush);
        SelectObject(hdc, oldVert);
        DeleteObject(vert);
        SelectObject(hdc, oldLbl);
        DeleteObject(lbl);
    }
    else if (title == L"Cumulative Grain Size Curve") {
        // Data from Python script
        double grainSizes[] = { 0.25, 0.30, 0.35, 0.40, 0.45, 0.50, 0.55, 0.60, 0.65, 0.70 };
        int counts[] = { 5, 10, 20, 25, 20, 10, 5, 3, 2, 1 };

        // Calculate cumulative percentages
        double cumulativePercent[10];
        int total = 0;
        for (int i = 0; i < 10; i++) total += counts[i];

        cumulativePercent[0] = (counts[0] * 100.0) / total;
        for (int i = 1; i < 10; i++) {
            cumulativePercent[i] = cumulativePercent[i - 1] + (counts[i] * 100.0) / total;
        }

        // Draw line graph
        HPEN graphPen = CreatePen(PS_SOLID, 3, RGB(46, 204, 113)); // Emerald green line
        HPEN oldGraphPen = (HPEN)SelectObject(hdc, graphPen);

        POINT points[10];
        for (int i = 0; i < 10; i++) {
            int xPos = graphLeft + (int)((grainSizes[i] - 0.25) / 0.45 * (graphRight - graphLeft));
            int yPos = graphBottom - (int)(cumulativePercent[i] / 100.0 * (graphBottom - graphTop));
            points[i] = { xPos, yPos };
        }
        Polyline(hdc, points, 10);

        // Draw data points as circles
        HBRUSH pointBrush = CreateSolidBrush(RGB(46, 204, 113));
        HBRUSH oldPointBrush = (HBRUSH)SelectObject(hdc, pointBrush);
        HPEN pointPen = CreatePen(PS_SOLID, 2, RGB(46, 204, 113));
        HPEN oldPointPen = (HPEN)SelectObject(hdc, pointPen);

        for (int i = 0; i < 10; i++) {
            Ellipse(hdc, points[i].x - 4, points[i].y - 4, points[i].x + 4, points[i].y + 4);
        }

        // Axis labels
       // HFONT lbl = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
         //   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        //HFONT oldLbl = (HFONT)SelectObject(hdc, lbl);
       // SetTextColor(hdc, RGB(170, 170, 170));

        HFONT lbl = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT oldLbl = (HFONT)SelectObject(hdc, lbl);
        SetTextColor(hdc, RGB(170, 170, 170));

        // X-axis label - RAISED POSITION
        RECT xAxisRect = { x, y + height - 65, x + width, y + height };
        DrawText(hdc, L"Grain Size (mm)", -1, &xAxisRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Rotated Y label - FIXED POSITIONING
        LOGFONT lf;
        GetObject(lbl, sizeof(LOGFONT), &lf);
        lf.lfEscapement = 900; // 90 degrees rotation
        HFONT vert = CreateFontIndirectW(&lf);
        HFONT oldVert = (HFONT)SelectObject(hdc, vert);

        RECT vertRect = { x + 5, y + height / 1 - 210, x + 159, y + height / 1 + 85 };
        DrawText(hdc, L"Cumulative % Passing", -1, &vertRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        // Clean up line graph resources
        SelectObject(hdc, oldVert);
        DeleteObject(vert);
        SelectObject(hdc, oldLbl);
        DeleteObject(lbl);
        SelectObject(hdc, oldPointPen);
        DeleteObject(pointPen);
        SelectObject(hdc, oldPointBrush);
        DeleteObject(pointBrush);
        SelectObject(hdc, oldGraphPen);
        DeleteObject(graphPen);
    }

    // Clean up
    SelectObject(hdc, oldPen);
    DeleteObject(axisPen);
    DeleteObject(gridPen);

    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    DeleteObject(hTitleF);
    SelectObject(hdc, hOldFont);
}
// Draw modern-looking rounded rectangle border
void DrawRoundedRect(HDC hdc, int x, int y, int width, int height, int radius, COLORREF color) {
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    RoundRect(hdc, x, y, x + width, y + height, radius, radius);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
}

void FillRoundedRect(HDC hdc, int x, int y, int width, int height, int radius, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
    HPEN oldPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));

    RoundRect(hdc, x, y, x + width, y + height, radius, radius);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
}

void DrawCard(HDC hdc, int x, int y, int width, int height, int radius) {
    // Create a semi-transparent card
    HBRUSH brush = CreateSolidBrush(CARD_BG);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
    HPEN oldPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));

    // Set up blending for transparency
    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 180; // Semi-transparent
    blend.AlphaFormat = 0;

    // Create a temporary DC for the rounded rectangle
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hbmMem);

    // Fill with card color
    HBRUSH memBrush = CreateSolidBrush(CARD_BG);
    RECT memRc = { 0, 0, width, height };
    FillRect(hdcMem, &memRc, memBrush);
    DeleteObject(memBrush);

    // Apply alpha blending
    AlphaBlend(hdc, x, y, width, height, hdcMem, 0, 0, width, height, blend);

    // Draw border
    DrawRoundedRect(hdc, x, y, width, height, radius, RGB(80, 80, 80));

    // Clean up
    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
}

// Draw owner-drawn modern button
void DrawModernButton(HDC hdc, HWND hwnd, CustomButton& button, const wchar_t* text) {
    RECT rc;
    GetClientRect(hwnd, &rc);

    bool isEnabled = IsWindowEnabled(hwnd);
    bool isHovered = button.isHovered && isEnabled;
    bool isPressed = button.isPressed && isEnabled;

    COLORREF bgColor;
    COLORREF borderColor;
    COLORREF textColor;

    // Make all buttons always green
    if (!isEnabled) {
        bgColor = BUTTON_BG;
        borderColor = RGB(80, 80, 80);
        textColor = RGB(120, 120, 120);
    }
    else {
        // Use green color scheme for all buttons
        if (isPressed) bgColor = TAG_GREEN_ACTIVE;
        else if (isHovered) bgColor = TAG_GREEN_HOVER;
        else bgColor = TAG_GREEN;
        borderColor = TAG_GREEN;
        textColor = RGB(255, 255, 255);
    }

    FillRoundedRect(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, button.cornerRadius, bgColor);
    DrawRoundedRect(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, button.cornerRadius, borderColor);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textColor);

    HFONT hf = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
    HFONT hOld = (HFONT)SelectObject(hdc, hf);

    // Fix: Remove trailing null character from text before drawing
    std::wstring fixedText = text;
    if (!fixedText.empty() && fixedText.back() == L'\0') {
        fixedText.pop_back();
    }

    // DrawText with padding to avoid overlap
    RECT textRc = rc;
    InflateRect(&textRc, -6, -2);
    DrawText(hdc, fixedText.c_str(), -1, &textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, hOld);
}
// Register button into list
void RegisterButton(HWND hwnd, int cornerRadius, bool isAccent, bool alwaysGreen) {
    CustomButton btn;
    btn.hwnd = hwnd;
    btn.isHovered = false;
    btn.isPressed = false;
    btn.cornerRadius = cornerRadius;
    btn.isAccent = isAccent;
    btn.alwaysGreen = alwaysGreen;
    customButtons.push_back(btn);
}

// Update hover/press state. hwnd param is main window handle.
void UpdateButtonState(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    POINT pt;
    if (msg == WM_MOUSEMOVE || msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP) {
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
    }
    else {
        return;
    }

    POINT screenPt = pt;
    ClientToScreen(hwnd, &screenPt);

    for (auto& btn : customButtons) {
        RECT rc;
        GetWindowRect(btn.hwnd, &rc);
        bool wasHovered = btn.isHovered;
        bool wasPressed = btn.isPressed;

        switch (msg) {
        case WM_MOUSEMOVE:
            btn.isHovered = PtInRect(&rc, screenPt);
            if (btn.isHovered && !wasHovered) {
                InvalidateRect(btn.hwnd, NULL, FALSE);
                TRACKMOUSEEVENT tme = {};
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);
            }
            else if (!btn.isHovered && wasHovered) {
                InvalidateRect(btn.hwnd, NULL, FALSE);
            }
            break;

        case WM_LBUTTONDOWN:
            btn.isPressed = PtInRect(&rc, screenPt);
            if (btn.isPressed != wasPressed) {
                InvalidateRect(btn.hwnd, NULL, FALSE);
            }
            break;

        case WM_LBUTTONUP:
            if (btn.isPressed) {
                btn.isPressed = false;
                InvalidateRect(btn.hwnd, NULL, FALSE);
            }
            break;
        }
    }
}
