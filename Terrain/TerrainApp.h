#pragma once

#include <windows.h>
#include <iostream>
#include <iomanip>
#include <ctime>

#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include "Include/d3dx12.h"
#include "Camera.h"

#define FRAMES_IN_FLIGHT 3

#define ENABLE_IMGUI 1

class TerrainApp 
{
public:

	TerrainApp();
	~TerrainApp();

	void Run();
	void Quit();

	void OnWindowResize(int width, int height);

private:

	void WaitForPreviousFrame();

	struct ConstantBuffer 
	{
		DirectX::XMFLOAT4X4 viewProj;
	};

	struct Vertex 
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 uv;
	};

	struct Image 
	{
		~Image();

		void LoadImageFromFile(const std::string& path, bool flip = true);

		char* Bytes = nullptr;
		int Width;
		int Height;
	};

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

	bool m_maximizeAtStart = false;
	bool m_isMouseLocked = true;
	unsigned char m_keys_state[256] = {};
	unsigned char m_old_keys_state[256] = {};
	Vec2 m_lastMousePos = {};
	bool m_first_time = true;

	HWND m_hwnd = nullptr;
	bool m_isRunning = false;
	float m_startTime = 0.0f;
	float m_lastTime = 0.0f;
	float m_timeElapsed = 0.0f;

	Camera m_camera = {};
	float m_cameraForward = 0.0f;
	float m_cameraRight = 0.0f;
	float m_cameraMoveSpeed = 16.0f;

	ID3D12Device* m_device = nullptr;
	IDXGISwapChain3* m_swapChain = nullptr;
	ID3D12CommandQueue* m_commandQueue = nullptr;
	ID3D12DescriptorHeap* m_rtvDescriptorHeap = nullptr;
	ID3D12Resource* m_renderTargets[FRAMES_IN_FLIGHT] = {};
	ID3D12CommandAllocator* m_commandAllocators[FRAMES_IN_FLIGHT] = {};
	ID3D12GraphicsCommandList* m_commandList = nullptr;
	ID3D12Fence* m_fences[FRAMES_IN_FLIGHT] = {};
	HANDLE m_fenceEvent = nullptr;
	UINT64 m_fenceValues[FRAMES_IN_FLIGHT] = {};
	int m_frameIndex = 0;
	int m_rtvDescriptorSize = 0;
	ID3D12PipelineState* m_pipelineStateObject = nullptr;
	ID3D12RootSignature* m_rootSignature = nullptr;
	D3D12_VIEWPORT m_viewport = {};
	D3D12_RECT m_scissorRect = {};
	ID3D12Resource* m_vertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView = {};
	int m_vertexCount = 0;
	ID3D12Resource* m_indexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView = {};
	int m_indexCount = 0;
	ID3D12Resource* m_depthStencilBuffer = nullptr;
	ID3D12DescriptorHeap* m_dsDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* m_mainDescriptorHeap[FRAMES_IN_FLIGHT] = {};
	ID3D12Resource* m_constantBufferUploadHeap[FRAMES_IN_FLIGHT] = {};
	ConstantBuffer m_constantBuffer = {};
	UINT8* m_constantBufferGPUAddress[FRAMES_IN_FLIGHT] = {};
	ID3D12Resource* m_textureBuffer = nullptr;
	ID3D12Resource* m_textureBufferUploadHeap = nullptr;
#if ENABLE_IMGUI
	static const int IMGUI_DESCRIPTOR_OFFSET = 10; // Start ImGui descriptors at offset 10
#endif
};