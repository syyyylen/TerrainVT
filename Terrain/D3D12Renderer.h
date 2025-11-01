#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include "Include/d3dx12.h"

#define FRAMES_IN_FLIGHT 3

class D3D12Renderer
{
public:
	D3D12Renderer(HWND hwnd, int width, int height);
	~D3D12Renderer();

	void Render();

private:
	void WaitForPreviousFrame();

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
};