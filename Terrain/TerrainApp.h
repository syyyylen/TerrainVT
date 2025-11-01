#pragma once

#include <windows.h>
#include <iostream>
#include <iomanip>
#include <ctime>

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

	bool s_isMouseLocked = true;
	unsigned char s_keys_state[256] = {};
	unsigned char s_old_keys_state[256] = {};
	Vec2 s_old_mouse_pos = {};
	bool s_first_time = true;

	HWND m_hwnd;
	bool m_isRunning;
	float m_startTime;
	float m_lastTime;
	float m_timeElapsed;
};