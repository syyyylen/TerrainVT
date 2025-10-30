#include "Terrain.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}
#endif

int main()
{
    std::cout << "Hello There !" << std::endl;

#ifdef _WIN32
    HINSTANCE hInstance = GetModuleHandleA(NULL);

    WNDCLASSEXA wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "TerrainWindowClass";

    if (!RegisterClassExA(&wc)) {
        std::cerr << "RegisterClassExA failed (" << GetLastError() << ")\n";
        return 1;
    }

    HWND hwnd = CreateWindowExA(
        0,
        wc.lpszClassName,
        "Terrain App",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1920, 1080,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        std::cerr << "CreateWindowExA failed (" << GetLastError() << ")\n";
        return 1;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    std::cout << "Window closed, exiting.\n";
#else
    std::cout << "This example creates a Win32 window; running on non-Windows platform.\n";
#endif

    std::cin.get();
    return 0;
}
