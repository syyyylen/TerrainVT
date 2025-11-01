#pragma once

#include <windows.h>
#include <iostream>
#include <iomanip>
#include <ctime>

#include "D3D12Renderer.h"

class TerrainApp 
{
public:

	TerrainApp();
	~TerrainApp();

	void Run();
	void Quit();

private:

	struct Vec2
	{
		Vec2() : x(0.0f), y(0.0f) {}
		Vec2(float _x, float _y) : x(_x), y(_y) {}
		float x, y;
	};

	void OnMouseMove(Vec2 pos);
	void OnKeyDown(int key);
	void OnKeyUp(int key);
	void OnLeftMouseDown(Vec2 pos);
	void OnRightMouseDown(Vec2 pos);
	void OnLeftMouseUp(Vec2 pos);
	void OnRightMouseUp(Vec2 pos);
	void UpdateInputs();

	bool m_isMouseLocked = false;
	unsigned char m_keys_state[256] = {};
	unsigned char m_old_keys_state[256] = {};
	Vec2 m_old_mouse_pos = {};
	bool m_first_time = true;

	HWND m_hwnd = nullptr;
	bool m_isRunning = false;
	float m_startTime = 0.0f;
	float m_lastTime = 0.0f;
	float m_timeElapsed = 0.0f;

	std::unique_ptr<D3D12Renderer> m_renderer = nullptr;
};