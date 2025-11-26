#pragma once
#include "Core.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <future>
#include "Camera.h"
#include "Texture.h"
#include "ShaderCompiler.h"
#include "VTex.h"

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

	void BakeHeightMap();
	void SaveHeightmapToPNG(const std::string& filepath);
	void BuildVTPageRequestResult(int frameIndex);

	// ------------------------ Render Data Structures ------------------------

	struct ConstantBuffer 
	{
		DirectX::XMFLOAT4X4 viewProj;
		float noise_persistence;
		float noise_lacunarity;
		float noise_scale;
		float noise_height;
		// 16 bytes
		int noise_octaves;
		int runtime_noise;
		int vt_texture_size;
		int vt_texture_page_size;
		// 16 bytes
		int vt_main_memory_texture_size;
		float vt_texture_tiling;
		DirectX::XMFLOAT2 padding;
	};

	struct Vertex 
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 uv;
	};

	struct VTPage
	{
		int mipMapLevel;
		std::pair<int, int> coords; // virtual coords in the source texture VTex (in page space)
		std::pair <UINT, UINT> physicalCoords; // physical coords in the gpu texture (in page/tile space, not pixel space)
	};

	struct VTPageRequest 
	{
		std::pair<int, int> requestedCoords;
		int requestedMipMapLevel;

		bool operator<(const VTPageRequest& other) const 
		{
			if (requestedMipMapLevel != other.requestedMipMapLevel)
				return requestedMipMapLevel < other.requestedMipMapLevel;

			return requestedCoords < other.requestedCoords;
		}
	};

	struct VTPageRequestResult 
	{
		std::set<VTPageRequest> requestedPages;
		std::vector<VTPage> loadedPages;
		int lastLoadedPageIdx;

		std::map<int, std::vector<ID3D12Resource*>> uploadHeaps;
		ID3D12Resource* pageTableUploadHeap;
	};

	// ------------------------ Input Handling ------------------------

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

	bool m_isMouseLocked = true;
	unsigned char m_keys_state[256] = {};
	unsigned char m_old_keys_state[256] = {};
	Vec2 m_lastMousePos = {};
	bool m_first_time = true;

	// ------------------------ App Global Settings ------------------------

	bool m_drawWireframe = false;
	bool m_maximizeAtStart = false;
	bool m_runtimeNoiseAtStart = false;
	std::string m_terrainTextureName = "rocks_albedo";

	HWND m_hwnd = nullptr;
	int m_width = 1920;
	int m_height = 1080;
	bool m_isRunning = false;
	float m_startTime = 0.0f;
	float m_lastTime = 0.0f;
	float m_timeElapsed = 0.0f;

	Camera m_camera = {};
	float m_cameraForward = 0.0f;
	float m_cameraRight = 0.0f;
	float m_cameraMoveSpeed = 320.0f;

	// ------------------------ D3D12 ------------------------

	ID3D12Device* m_device = nullptr;
	IDXGISwapChain3* m_swapChain = nullptr;
	ID3D12CommandQueue* m_graphicsCommandQueue = nullptr;
	ID3D12DescriptorHeap* m_rtvDescriptorHeap = nullptr;
	ID3D12Resource* m_renderTargets[FRAMES_IN_FLIGHT] = {};
	ID3D12CommandAllocator* m_commandAllocators[FRAMES_IN_FLIGHT] = {};
	ID3D12GraphicsCommandList* m_commandList = nullptr;
	int m_rtvDescriptorSize = 0;
	ID3D12PipelineState* m_pipelineStateObject = nullptr;
	ID3D12RootSignature* m_rootSignature = nullptr;
	ID3D12Resource* m_depthStencilBuffer = nullptr;
	ID3D12DescriptorHeap* m_dsDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* m_mainDescriptorHeap[FRAMES_IN_FLIGHT] = {};
	D3D12_VIEWPORT m_viewport = {};
	D3D12_RECT m_scissorRect = {};

	// Fence
	ID3D12Fence* m_fences[FRAMES_IN_FLIGHT] = {};
	HANDLE m_fenceEvent = nullptr;
	UINT64 m_fenceValues[FRAMES_IN_FLIGHT] = {};
	int m_frameIndex = 0;

	// VB/IB
	ID3D12Resource* m_vertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView = {};
	int m_vertexCount = 0;
	ID3D12Resource* m_indexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView = {};
	int m_indexCount = 0;

	// Constant Buffer
	ID3D12Resource* m_constantBufferUploadHeap[FRAMES_IN_FLIGHT] = {};
	ConstantBuffer m_constantBuffer = {};
	UINT8* m_constantBufferGPUAddress[FRAMES_IN_FLIGHT] = {};

	// Compute Pipeline
	ID3D12RootSignature* m_computeRootSignature = nullptr;
	ID3D12PipelineState* m_computePipelineState = nullptr;
	ID3D12Resource* m_heightmapReadbackBuffer = nullptr;
	bool m_saveHeightmapAfterFrame = false;

	// Copy Queue
	/*ID3D12CommandQueue* m_copyCommandQueue;
	ID3D12CommandAllocator* m_copyCommandAllocators[FRAMES_IN_FLIGHT] = {};
	ID3D12GraphicsCommandList* m_copyCommandList;*/

	// VT
	int m_vtMemoryBudget = 2048;
	int m_vtPageSize = 256;
	ID3D12PipelineState* m_renderToTexturePSO = nullptr;
	ID3D12Resource* m_VTpagesRequestReadBackBuffer[FRAMES_IN_FLIGHT] = {};
	VTPageRequestResult m_VTpagesRequestResult = {};
	bool m_hasVTpageRequestPending[FRAMES_IN_FLIGHT] = {};
	std::atomic<bool> m_buildVTResultInProgress{false};
	std::future<void> m_buildVTResultFuture;
	Texture m_VTMainMemoryTexture = {};
	Texture m_VTPageTableTexture = {};

	Texture m_renderTexture = {}; // TODO resolution / 2 this texture
	Texture m_albedoTexture = {};
	Texture m_computeOutputTexture = {};
	Texture m_bakedHeightmapTexture = {};

#if ENABLE_IMGUI
	static const int IMGUI_DESCRIPTOR_OFFSET = 30;
#endif
};