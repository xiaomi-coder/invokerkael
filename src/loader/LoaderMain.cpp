#include "../Includes.h"
#include "Injector.h"
#include <filesystem>

// -----------------------------------------------------------------------
// SkinLoader.exe
//
// Reuses the existing login window, then shows a single plain Win32 window
// with a START button that injects SkinCore.dll into cs2.exe.
// -----------------------------------------------------------------------

namespace
{
    constexpr int ID_BUTTON_START = 1001;

    constexpr const char* DLL_NAME      = "SkinCore.dll";
    constexpr const char* GAME_PROCESS  = "cs2.exe";
    constexpr const char* WINDOW_TITLE  = "KAEL_CHEAT - SkinCore";

    HWND g_hStatus   = nullptr;
    HWND g_hButton   = nullptr;
    HFONT g_hFont    = nullptr;
    HFONT g_hFontBig = nullptr;
    bool g_bInjected = false;

    void SetStatus(const char* szText)
    {
        if (g_hStatus)
            SetWindowTextA(g_hStatus, szText);
    }

    void OnStartClicked(HWND hWnd)
    {
        std::cout << "[OnStartClicked] Called\n";

        if (g_bInjected)
        {
            std::cout << "[OnStartClicked] Already injected\n";
            SetStatus("Allaqachon ishga tushirilgan.");
            return;
        }

        std::cout << "[OnStartClicked] Starting injection...\n";
        SetStatus("Ishga tushirilmoqda...");
        EnableWindow(g_hButton, FALSE);

        try
        {
            const std::string strDllPath = Injector::GetPathNextToExe(DLL_NAME);
            std::cout << "[OnStartClicked] DLL path: " << strDllPath << "\n";

            std::string strError;
            std::cout << "[OnStartClicked] Calling Injector::Inject...\n";
            std::cout.flush();

            bool bInjectResult = Injector::Inject(GAME_PROCESS, strDllPath, strError);
            std::cout << "[OnStartClicked] Inject returned: " << (bInjectResult ? "true" : "false") << "\n";
            std::cout << "[OnStartClicked] Error string: " << strError << "\n";
            std::cout.flush();

            char szFullMsg[2048]{};
            snprintf(szFullMsg, sizeof(szFullMsg),
                     "DLL path:\n%s\n\nDLL mavjud: %s\n\n"
                     "Injection natijasi: %s\n\n%s",
                     strDllPath.c_str(),
                     std::filesystem::exists(strDllPath) ? "Ha" : "Yo'q",
                     bInjectResult ? "MUVAFFAQ!" : "XATO",
                     bInjectResult ? "Skins + Glow faol!" : strError.c_str());

            std::cout << "[OnStartClicked] Showing MessageBox...\n";
            std::cout.flush();
            MessageBoxA(hWnd, szFullMsg, "KAEL_CHEAT",
                        bInjectResult ? (MB_OK | MB_ICONINFORMATION) : (MB_OK | MB_ICONERROR));

            if (bInjectResult)
            {
                g_bInjected = true;
                SetStatus("Ishga tushdi! Skin va glow faol.");
                SetWindowTextA(g_hButton, "ISHLAYAPTI");
            }
            else
            {
                SetStatus(strError.c_str());
                EnableWindow(g_hButton, TRUE);
            }
            std::cout << "[OnStartClicked] Done\n";
        }
        catch (const std::exception& e)
        {
            std::cout << "[OnStartClicked] Exception caught: " << e.what() << "\n";
            std::cout.flush();
            char szMsg[1024]{};
            snprintf(szMsg, sizeof(szMsg), "Exception:\n%s", e.what());
            MessageBoxA(hWnd, szMsg, "Error", MB_OK | MB_ICONERROR);
            EnableWindow(g_hButton, TRUE);
        }
        catch (...)
        {
            std::cout << "[OnStartClicked] Unknown exception caught\n";
            std::cout.flush();
            MessageBoxA(hWnd, "Unknown exception in Inject", "Error", MB_OK | MB_ICONERROR);
            EnableWindow(g_hButton, TRUE);
        }
    }

    LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_BUTTON_START)
            {
                OnStartClicked(hWnd);
                return 0;
            }
            break;

        case WM_CTLCOLORSTATIC:
        {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetTextColor(hdc, RGB(220, 220, 230));
            SetBkColor(hdc, RGB(24, 24, 32));
            static HBRUSH hBrush = CreateSolidBrush(RGB(24, 24, 32));
            return reinterpret_cast<LRESULT>(hBrush);
        }

        case WM_CTLCOLORBTN:
        {
            static HBRUSH hBrush = CreateSolidBrush(RGB(24, 24, 32));
            return reinterpret_cast<LRESULT>(hBrush);
        }

        case WM_ERASEBKGND:
        {
            HDC  hdc = reinterpret_cast<HDC>(wParam);
            RECT rc{};
            GetClientRect(hWnd, &rc);
            HBRUSH hBrush = CreateSolidBrush(RGB(24, 24, 32));
            FillRect(hdc, &rc, hBrush);
            DeleteObject(hBrush);
            return 1;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }

    bool CreateLoaderWindow()
    {
        WNDCLASSEXA wc{};
        wc.cbSize        = sizeof(WNDCLASSEXA);
        wc.lpfnWndProc   = WndProc;
        wc.hInstance     = GetModuleHandleA(nullptr);
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = "ShiftHubSkinLoader";

        if (!RegisterClassExA(&wc))
            return false;

        const int iWidth  = 380;
        const int iHeight = 210;
        const int iX = (GetSystemMetrics(SM_CXSCREEN) - iWidth)  / 2;
        const int iY = (GetSystemMetrics(SM_CYSCREEN) - iHeight) / 2;

        HWND hWnd = CreateWindowExA(
            0, wc.lpszClassName, WINDOW_TITLE,
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            iX, iY, iWidth, iHeight,
            nullptr, nullptr, wc.hInstance, nullptr);

        if (!hWnd)
            return false;

        g_hFont = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

        g_hFontBig = CreateFontA(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

        HWND hTitle = CreateWindowA("STATIC", "CS2 ni yoqing, keyin START bosing",
                                    WS_CHILD | WS_VISIBLE | SS_CENTER,
                                    20, 20, 330, 24, hWnd, nullptr, wc.hInstance, nullptr);
        SendMessageA(hTitle, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFont), TRUE);

        g_hButton = CreateWindowA("BUTTON", "START",
                                  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                  90, 62, 190, 48, hWnd,
                                  reinterpret_cast<HMENU>(ID_BUTTON_START), wc.hInstance, nullptr);
        SendMessageA(g_hButton, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFontBig), TRUE);

        g_hStatus = CreateWindowA("STATIC", "Tayyor.",
                                  WS_CHILD | WS_VISIBLE | SS_CENTER,
                                  20, 126, 330, 40, hWnd, nullptr, wc.hInstance, nullptr);
        SendMessageA(g_hStatus, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFont), TRUE);

        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
        return true;
    }
}

// -----------------------------------------------------------------------
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInstance, LPSTR pArgs, int iCmdShow)
{
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    std::cout << "[SkinLoader] Ishga tushdi\n";

    try
    {
        // Same CWD fix the main executable uses, so SkinCore.dll resolves even
        // when launched from a shortcut or a browser download.
        char szPath[MAX_PATH];
        if (GetModuleFileNameA(nullptr, szPath, MAX_PATH))
        {
            std::string strPath = szPath;
            const size_t pos = strPath.find_last_of("\\/");
            if (pos != std::string::npos)
                SetCurrentDirectoryA(strPath.substr(0, pos).c_str());
        }

        g_Globals.m_hDll = hInstance;
        std::cout << "[SkinLoader] g_Globals initialized\n";

        // No login here — this tool goes straight to the START button.

        if (!CreateLoaderWindow())
        {
            std::cout << "[SkinLoader] CreateLoaderWindow failed\n";
            MessageBoxA(nullptr, "Oyna yaratib bo'lmadi.", "KAEL_CHEAT - Xato", MB_OK | MB_ICONERROR);
            return EXIT_FAILURE;
        }

        std::cout << "[SkinLoader] Window created, entering message loop\n";

        MSG msg{};
        while (GetMessageA(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        if (g_hFont)    DeleteObject(g_hFont);
        if (g_hFontBig) DeleteObject(g_hFontBig);

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        std::cout << "[SkinLoader] Exception: " << e.what() << "\n";
        MessageBoxA(nullptr, e.what(), "Exception", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cout << "[SkinLoader] Unknown exception\n";
        MessageBoxA(nullptr, "Unknown exception", "Error", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }
}
