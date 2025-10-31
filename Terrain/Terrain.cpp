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

// ---------------------------------------- Input Handling ----------------------------------------

struct Vec2
{
    Vec2() : x(0.0f), y(0.0f) {}
    Vec2(float _x, float _y) : x(_x), y(_y) {}
    float x, y;
};

static bool s_isMouseLocked = true;
static unsigned char s_keys_state[256] = {};
static unsigned char s_old_keys_state[256] = {};
static Vec2 s_old_mouse_pos = {};
static bool s_first_time = true;

void OnMouseMove(Vec2 pos)
{
    if (!s_isMouseLocked)
        return;
}

void OnKeyDown(int key){}

void OnKeyUp(int key)
{
    if (key == 'E')
    {
        s_isMouseLocked = s_isMouseLocked ? false : true;
        ::ShowCursor(!s_isMouseLocked);
        RECT rc;
        ::GetClientRect(s_hwnd, &rc);
        ::SetCursorPos((rc.right - rc.left) / 2.0f, (rc.bottom - rc.top) / 2.0f);
    }
}

void OnLeftMouseDown(Vec2 pos) {}
void OnRightMouseDown(Vec2 pos) {}
void OnLeftMouseUp(Vec2 pos) {}
void OnRightMouseUp(Vec2 pos) {}

void UpdateInputs()
{
    POINT current_mouse_pos = {};
    ::GetCursorPos(&current_mouse_pos);

    if (s_first_time)
    {
        s_old_mouse_pos = Vec2(current_mouse_pos.x, current_mouse_pos.y);
        s_first_time = false;
    }

    if (current_mouse_pos.x != s_old_mouse_pos.x || current_mouse_pos.y != s_old_mouse_pos.y)
    {
        ::OnMouseMove(Vec2(current_mouse_pos.x, current_mouse_pos.y));
    }

    s_old_mouse_pos = Vec2(current_mouse_pos.x, current_mouse_pos.y);

    if (::GetKeyboardState(s_keys_state))
    {
        for (unsigned int i = 0; i < 256; i++)
        {
            if (s_keys_state[i] & 0x80)
            {
                if (i == VK_LBUTTON)
                {
                    if (s_keys_state[i] != s_old_keys_state[i])
                        ::OnLeftMouseDown(Vec2(current_mouse_pos.x, current_mouse_pos.y));
                }
                else if (i == VK_RBUTTON)
                {
                    if (s_keys_state[i] != s_old_keys_state[i])
                        ::OnRightMouseDown(Vec2(current_mouse_pos.x, current_mouse_pos.y));
                }
                else
                    ::OnKeyDown(i);
            }
            else
            {
                if (s_keys_state[i] != s_old_keys_state[i])
                {
                    if (i == VK_LBUTTON)
                        ::OnLeftMouseUp(Vec2(current_mouse_pos.x, current_mouse_pos.y));
                    else if (i == VK_RBUTTON)
                        ::OnRightMouseUp(Vec2(current_mouse_pos.x, current_mouse_pos.y));
                    else
                        ::OnKeyUp(i);
                }

            }
        }
        ::memcpy(s_old_keys_state, s_keys_state, sizeof(unsigned char) * 256);
    }
}

// ---------------------------------------- Input Handling ----------------------------------------

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

    s_startTime = static_cast<float>(::clock());

#ifdef _WIN32

    HINSTANCE hInstance = ::GetModuleHandleW(NULL);

    WNDCLASSEXW wc{};
    wc.cbClsExtra = NULL;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.cbWndExtra = NULL;
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    wc.hInstance = NULL;
    wc.lpszClassName = L"Terrain App";
    wc.lpszMenuName = L"";
    wc.style = NULL;
    wc.lpfnWndProc = &WndProc;

    if (!::RegisterClassExW(&wc))
    {
        std::cerr << "RegisterClassExW failed (" << ::GetLastError() << ")\n";
        return 1;
    }

    int width = 1920;
    int height = 1080;

    s_hwnd = ::CreateWindowExW(
        WS_EX_OVERLAPPEDWINDOW,
        wc.lpszClassName,
        L"Terrain App",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, hInstance, NULL);

    if (!s_hwnd)
    {
        std::cerr << "CreateWindowExW failed (" << ::GetLastError() << ")\n";
        return 1;
    }

    ::ShowWindow(s_hwnd, SW_SHOW);
    ::UpdateWindow(s_hwnd);
    ::ShowCursor(false);
    int centerX = width / 2;
    int centerY = height / 2;
    ::SetCursorPos(centerX, centerY);

    s_isRunning = true;

    while (s_isRunning)
    {
        float time = static_cast<float>(clock()) - s_startTime;
        float dt = (time - s_lastTime) / 1000.0f;
        s_lastTime = time;
        s_timeElapsed += dt;

        // std::cout << "Time Elapsed : " << std::fixed << std::setprecision(1) << s_timeElapsed << "s" << std::endl;

        UpdateInputs();

        MSG msg{};
        while (::PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE) > 0)
        {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
    }

    ::DestroyWindow(s_hwnd);

#endif

    return 0;
}
