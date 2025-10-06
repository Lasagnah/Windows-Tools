// techdom example.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include <string>
#include <fstream>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")

#define ID_CHECKBOX1 101
#define ID_CHECKBOX2 102
#define ID_CHECKBOX3 103
#define ID_TIMER     104

HWND hCheckbox1, hCheckbox2, hCheckbox3;

int changeBackground(const char* link, BOOL online) {
    char localPath[MAX_PATH] = { 0 };

    if (online) {
        // Download image from URL and save locally
        HINTERNET hInternet = InternetOpenA("WallpaperDownloader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) {
            std::cerr << "InternetOpenA failed." << std::endl;
            return -1;
        }

        HINTERNET hFile = InternetOpenUrlA(hInternet, link, NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (!hFile) {
            std::cerr << "InternetOpenUrlA failed." << std::endl;
            InternetCloseHandle(hInternet);
            return -1;
        }

        // Get temp path and create a filename
        if (!GetTempPathA(MAX_PATH, localPath)) {
            std::cerr << "GetTempPathA failed." << std::endl;
            InternetCloseHandle(hFile);
            InternetCloseHandle(hInternet);
            return -1;
        }
        strcat_s(localPath, MAX_PATH, "temp_wallpaper.jpg");

        // Open local file for writing
        std::ofstream output(localPath, std::ios::binary);
        if (!output.is_open()) {
            std::cerr << "Failed to open local file for writing." << std::endl;
            InternetCloseHandle(hFile);
            InternetCloseHandle(hInternet);
            return -1;
        }

        // Download loop
        char buffer[4096];
        DWORD bytesRead = 0;
        while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead != 0) {
            output.write(buffer, bytesRead);
        }

        output.close();
        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);

        // Now point to the downloaded local file
        link = localPath;
    }
    
    // Set the wallpaper
    BOOL result = SystemParametersInfoA(
        SPI_SETDESKWALLPAPER,  // Action: Set the wallpaper
        0,                     // Unused for setting wallpaper
        (PVOID)link,      // Path to the image file
        SPIF_UPDATEINIFILE | SPIF_SENDCHANGE // Flags: Update user profile and broadcast change
    );

    if (result) {
        std::cout << "Wallpaper changed successfully!" << std::endl;
        return 0;
    }
    else {
        std::cout << "Failed to change wallpaper." << std::endl;
        return -1;
    }
}

// Feature actions
void DoFeature1() {
    changeBackground("C:\\Users\\matts\\Downloads\\ffxiv wallpaper.bmp", FALSE);
    OutputDebugString(L"Background Changed\n");
}
void DoFeature2() {
    OutputDebugString(L"Feature 2 is active\n");
}
void DoFeature3() {
    OutputDebugString(L"Feature 3 is active\n");
}

// Run program: Ctrl + F5 or Debug > Start Withou
// 
// 
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        // Set up a timer that ticks every 1000ms (1 second)
        SetTimer(hwnd, ID_TIMER, 60000, NULL);
        return 0;

    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED) {
            // Optional: show checkbox state on click
            HWND checkbox = (HWND)lParam;
            BOOL checked = SendMessage(checkbox, BM_GETCHECK, 0, 0) == BST_CHECKED;
            int controlID = LOWORD(wParam);

            std::wstring msg;
            switch (controlID) {
            case ID_CHECKBOX1:
                msg = L"Feature 1 " + std::wstring(checked ? L"Enabled" : L"Disabled");
                OutputDebugString(L"Feature 1 enabled\n");
                break;
            case ID_CHECKBOX2:
                msg = L"Feature 2 " + std::wstring(checked ? L"Enabled" : L"Disabled");
                OutputDebugString(L"Feature 2 enabled\n");
                break;
            case ID_CHECKBOX3:
                msg = L"Feature 3 " + std::wstring(checked ? L"Enabled" : L"Disabled");
                OutputDebugString(L"Feature 3 enabled\n");
                break;
            }
            MessageBox(hwnd, msg.c_str(), L"Feature Toggle", MB_OK | MB_ICONINFORMATION);
        }
        return 0;

    case WM_TIMER:
        if (SendMessage(hCheckbox1, BM_GETCHECK, 0, 0) == BST_CHECKED) 
            DoFeature1(); 
        if (SendMessage(hCheckbox2, BM_GETCHECK, 0, 0) == BST_CHECKED) 
            DoFeature2(); 
        if (SendMessage(hCheckbox3, BM_GETCHECK, 0, 0) == BST_CHECKED) 
            DoFeature3();
        return 0;


    case WM_DESTROY:
        KillTimer(hwnd, ID_TIMER);
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"FeatureToggleWindowClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles
        CLASS_NAME,                     // Window class
        L"Feature Toggle GUI",          // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 250,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    if (hwnd == NULL) {
        return 0;
    }

    // Create checkboxes
    hCheckbox1 = CreateWindow(L"BUTTON", L"Feature 1",
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        20, 30, 150, 30,
        hwnd, (HMENU)ID_CHECKBOX1, hInstance, NULL);

    hCheckbox2 = CreateWindow(L"BUTTON", L"Feature 2",
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        20, 70, 150, 30,
        hwnd, (HMENU)ID_CHECKBOX2, hInstance, NULL);

    hCheckbox3 = CreateWindow(L"BUTTON", L"Feature 3",
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        20, 110, 150, 30,
        hwnd, (HMENU)ID_CHECKBOX3, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);

    // Run the message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
