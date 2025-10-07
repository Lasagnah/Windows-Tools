#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include <string>
#include <fstream>
#include <wininet.h>
#include "resource.h"
#include <vector>
#include <random>
#include <fstream>
#pragma comment(lib, "wininet.lib")

#define ID_CHECKBOX1 101
#define ID_CHECKBOX2 102
#define ID_CHECKBOX3 103
#define ID_TIMER     104

HWND hCheckbox1, hCheckbox2, hCheckbox3; 
bool isUnlocked = false;
const std::wstring correctPassword = L"1234"; // Password
// Remember previous states of checkboxes to revert if password fails
BOOL prevStateCheckbox1 = BST_UNCHECKED;
BOOL prevStateCheckbox2 = BST_UNCHECKED;
BOOL prevStateCheckbox3 = BST_UNCHECKED;
std::vector<std::string> links = {};

int getLinks() {
    std::ifstream file("links.txt");

    if (!file.is_open()) {
        OutputDebugStringW(L"Failed to Open File (Links).\n");
        return 1;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {  // Optional: skip empty lines
            links.push_back(line);
        }
    }

    file.close();  // Always a good habit
    OutputDebugStringW(L"Got Links.\n");
    return 0;
}

bool PromptForPassword(HWND parent) {
    wchar_t buffer[256] = { 0 };

    //OutputDebugStringW(L"Correct password: '");
    //OutputDebugStringW(correctPassword.c_str());
    //OutputDebugStringW(L"'\n");

    if (DialogBoxParam(nullptr, MAKEINTRESOURCE(IDD_PASSWORD_DIALOG), parent,
        [](HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) -> INT_PTR {

            switch (msg) {
            case WM_INITDIALOG:
                // Store the buffer pointer in dialog user data for later access
                SetWindowLongPtr(hDlg, DWLP_USER, lParam);
                return TRUE;

            case WM_COMMAND:
                if (LOWORD(wParam) == IDOK) {
                    // Retrieve buffer pointer from dialog user data
                    wchar_t* pBuffer = (wchar_t*)GetWindowLongPtr(hDlg, DWLP_USER);

                    // Get the text from the password input control (ID 1001 assumed)
                    GetDlgItemText(hDlg, 1001, pBuffer, 256);

                    //OutputDebugStringW(L"Raw entered password: '");
                    //OutputDebugStringW(pBuffer);
                    //OutputDebugStringW(L"'\n");

                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }
                else if (LOWORD(wParam) == IDCANCEL) {
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
                }
                break;
            }

            return FALSE;
        }, (LPARAM)buffer) == IDOK) {

        std::wstring entered(buffer);

        // Trim spaces
        auto start = entered.find_first_not_of(L" \t\r\n");
        auto end = entered.find_last_not_of(L" \t\r\n");
        std::wstring trimmed = (start != std::wstring::npos && end != std::wstring::npos) ?
            entered.substr(start, end - start + 1) : L"";

        //OutputDebugStringW(L"Trimmed password: '");
        //OutputDebugStringW(trimmed.c_str());
        //OutputDebugStringW(L"'\n");

        return trimmed == correctPassword;
    }
    return false;
}


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
    std::random_device rd;  // Non-deterministic seed
    std::mt19937 gen(rd()); // Mersenne Twister engine
    std::uniform_int_distribution<> dist(0, links.size() - 1);

    // Step 3: Pick a random link
    std::string random_link = links[dist(gen)];

    changeBackground(random_link.c_str(), TRUE);
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
        SetTimer(hwnd, ID_TIMER, 1000, NULL);
        return 0;

    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED) {
            // Stop timer so features won't run during password prompt
            KillTimer(hwnd, ID_TIMER);

            HWND checkbox = (HWND)lParam;
            int controlID = LOWORD(wParam);
            BOOL checked = SendMessage(checkbox, BM_GETCHECK, 0, 0) == BST_CHECKED;

            // Prompt for password
            if (!PromptForPassword(hwnd)) {
                MessageBox(hwnd, L"Incorrect password.", L"Access Denied", MB_ICONERROR);

                // Revert checkbox state to previous
                switch (controlID) {
                case ID_CHECKBOX1:
                    SendMessage(checkbox, BM_SETCHECK, prevStateCheckbox1, 0);
                    break;
                case ID_CHECKBOX2:
                    SendMessage(checkbox, BM_SETCHECK, prevStateCheckbox2, 0);
                    break;
                case ID_CHECKBOX3:
                    SendMessage(checkbox, BM_SETCHECK, prevStateCheckbox3, 0);
                    break;
                }

                // Restart timer
                SetTimer(hwnd, ID_TIMER, 1000, NULL);
                return 0;
            }

            // Password correct: update previous state to current
            switch (controlID) {
            case ID_CHECKBOX1:
                prevStateCheckbox1 = checked ? BST_CHECKED : BST_UNCHECKED;
                break;
            case ID_CHECKBOX2:
                prevStateCheckbox2 = checked ? BST_CHECKED : BST_UNCHECKED;
                break;
            case ID_CHECKBOX3:
                prevStateCheckbox3 = checked ? BST_CHECKED : BST_UNCHECKED;
                break;
            }

            // Show message about feature status
            std::wstring msg;
            switch (controlID) {
            case ID_CHECKBOX1:
                msg = L"Background Swapping " + std::wstring(checked ? L"Enabled" : L"Disabled");
                OutputDebugString(L"Feature 1 (Background swapping) ");
                OutputDebugString(checked ? L"enabled\n" : L"disabled\n");
                break;
            case ID_CHECKBOX2:
                msg = L"Feature 2 " + std::wstring(checked ? L"Enabled" : L"Disabled");
                OutputDebugString(L"Feature 2 ");
                OutputDebugString(checked ? L"enabled\n" : L"disabled\n");
                break;
            case ID_CHECKBOX3:
                msg = L"Feature 3 " + std::wstring(checked ? L"Enabled" : L"Disabled");
                OutputDebugString(L"Feature 3 ");
                OutputDebugString(checked ? L"enabled\n" : L"disabled\n");
                break;
            }

            MessageBox(hwnd, msg.c_str(), L"Techdom Example", MB_OK | MB_ICONINFORMATION);

            // Restart timer
            SetTimer(hwnd, ID_TIMER, 1000, NULL);
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
    
    getLinks();

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles
        CLASS_NAME,                     // Window class
        L"Techdom Toggles GUI",          // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 250,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    prevStateCheckbox1 = BST_UNCHECKED;
    prevStateCheckbox2 = BST_UNCHECKED;
    prevStateCheckbox3 = BST_UNCHECKED;

    if (hwnd == NULL) {
        return 0;
    }

    // Create checkboxes
    hCheckbox1 = CreateWindow(L"BUTTON", L"Background Swaps",
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