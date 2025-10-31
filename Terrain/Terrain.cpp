#include "Terrain.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <iomanip>
#include <ctime>

static HWND s_hwnd;
static bool s_isRunning;
static float s_startTime;
static float s_lastTime;
static float s_timeElapsed;

#ifdef _WIN32
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
    {
        break;
    }

    case WM_SETFOCUS:
    {
        break;
    }

    case WM_KILLFOCUS:
    {
        break;
    }

    case WM_DESTROY:
    {
        s_isRunning = false;
        ::PostQuitMessage(0);
        break;
    }

    default:
        return ::DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    return 0;
}
#endif

int main()
{
    std::cout << "Hello There !" << std::endl;

    s_startTime = static_cast<float>(clock());

#ifdef _WIN32
    HINSTANCE hInstance = GetModuleHandleW(NULL);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"TerrainWindowClass";

    if (!RegisterClassExW(&wc))
    {
        std::cerr << "RegisterClassExW failed (" << GetLastError() << ")\n";
        return 1;
    }

    int width = 1920;
    int height = 1080;

    s_hwnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"Terrain App",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, hInstance, NULL);

    if (!s_hwnd)
    {
        std::cerr << "CreateWindowExW failed (" << GetLastError() << ")\n";
        return 1;
    }

    ShowWindow(s_hwnd, SW_SHOW);
    UpdateWindow(s_hwnd);
    int centerX = width / 2;
    int centerY = height / 2;
    SetCursorPos(centerX, centerY);

    s_isRunning = true;

    while (s_isRunning)
    {
        float time = static_cast<float>(clock()) - s_startTime;
        float dt = (time - s_lastTime) / 1000.0f;
        s_lastTime = time;
        s_timeElapsed += dt;

        std::cout << "Time Elapsed : " << std::fixed << std::setprecision(1) << s_timeElapsed << "s" << std::endl;

        MSG msg{};
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    DestroyWindow(s_hwnd);

#endif

    return 0;
}
