#include "TerrainApp.h"

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TerrainApp* app = reinterpret_cast<TerrainApp*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));

	switch (msg)
	{
	case WM_SIZE:
	{
		break;
	}

	case WM_CLOSE:
	{
		if (app)
			app->Quit();
		break;
	}

	case WM_DESTROY:
	{
		if (app)
			app->Quit();
		break;
	}

	default:
		return ::DefWindowProcW(hwnd, msg, wParam, lParam);
	}

	return 0;
}

TerrainApp::TerrainApp()
{
	m_startTime = static_cast<float>(::clock());

	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpszClassName = L"TerrainAppWindowClass";
	wc.lpfnWndProc = &WndProc;

	if (!::RegisterClassExW(&wc))
	{
		std::cerr << "RegisterClassExW failed (" << ::GetLastError() << ")\n";
		return;
	}

	int width = 1920;
	int height = 1080;

	m_hwnd = ::CreateWindowExW(
		WS_EX_OVERLAPPEDWINDOW,
		wc.lpszClassName,
		L"Terrain App",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, width, height,
		nullptr, nullptr, nullptr, this);

	if (!m_hwnd)
	{
		std::cerr << "CreateWindowExW failed (" << ::GetLastError() << ")\n";
		return;
	}

	::SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

	::ShowWindow(m_hwnd, SW_SHOW);
	::UpdateWindow(m_hwnd);
	::ShowCursor(!m_isMouseLocked);
	int centerX = width / 2;
	int centerY = height / 2;
	::SetCursorPos(centerX, centerY);

	m_renderer = std::make_unique<D3D12Renderer>();

	m_isRunning = true;
}

TerrainApp::~TerrainApp()
{
	m_renderer.reset();

	::DestroyWindow(m_hwnd);
}

void TerrainApp::Run()
{
	while (m_isRunning)
	{
		float time = static_cast<float>(clock()) - m_startTime;
		float dt = (time - m_lastTime) / 1000.0f;
		m_lastTime = time;
		m_timeElapsed += dt;

		UpdateInputs();

		MSG msg{};
		while (::PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE) > 0)
		{
			::TranslateMessage(&msg);
			::DispatchMessageW(&msg);
		}
	}
}

void TerrainApp::Quit()
{
	m_isRunning = false;
}

void TerrainApp::OnMouseMove(Vec2 pos)
{
	if (!m_isMouseLocked)
		return;
}

void TerrainApp::OnKeyDown(int key) {}

void TerrainApp::OnKeyUp(int key)
{
	if (key == 'E')
	{
		m_isMouseLocked = m_isMouseLocked ? false : true;
		::ShowCursor(!m_isMouseLocked);
		RECT rc;
		::GetClientRect(m_hwnd, &rc);
		::SetCursorPos((rc.right - rc.left) / 2.0f, (rc.bottom - rc.top) / 2.0f);
	}
}

void TerrainApp::OnLeftMouseDown(Vec2 pos) {}
void TerrainApp::OnRightMouseDown(Vec2 pos) {}
void TerrainApp::OnLeftMouseUp(Vec2 pos) {}
void TerrainApp::OnRightMouseUp(Vec2 pos) {}

void TerrainApp::UpdateInputs()
{
	POINT current_mouse_pos = {};
	::GetCursorPos(&current_mouse_pos);

	if (m_first_time)
	{
		m_old_mouse_pos = Vec2(current_mouse_pos.x, current_mouse_pos.y);
		m_first_time = false;
	}

	if (current_mouse_pos.x != m_old_mouse_pos.x || current_mouse_pos.y != m_old_mouse_pos.y)
	{
		OnMouseMove(Vec2(current_mouse_pos.x, current_mouse_pos.y));
	}

	m_old_mouse_pos = Vec2(current_mouse_pos.x, current_mouse_pos.y);

	if (::GetKeyboardState(m_keys_state))
	{
		for (unsigned int i = 0; i < 256; i++)
		{
			if (m_keys_state[i] & 0x80)
			{
				if (i == VK_LBUTTON)
				{
					if (m_keys_state[i] != m_old_keys_state[i])
						OnLeftMouseDown(Vec2(current_mouse_pos.x, current_mouse_pos.y));
				}
				else if (i == VK_RBUTTON)
				{
					if (m_keys_state[i] != m_old_keys_state[i])
						OnRightMouseDown(Vec2(current_mouse_pos.x, current_mouse_pos.y));
				}
				else
					OnKeyDown(i);
			}
			else
			{
				if (m_keys_state[i] != m_old_keys_state[i])
				{
					if (i == VK_LBUTTON)
						OnLeftMouseUp(Vec2(current_mouse_pos.x, current_mouse_pos.y));
					else if (i == VK_RBUTTON)
						OnRightMouseUp(Vec2(current_mouse_pos.x, current_mouse_pos.y));
					else
						OnKeyUp(i);
				}

			}
		}
		::memcpy(m_old_keys_state, m_keys_state, sizeof(unsigned char) * 256);
	}
}