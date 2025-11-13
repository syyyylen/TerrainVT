#include "TerrainApp.h"

#if ENABLE_IMGUI
#include "Include/ImGui/imgui_impl_dx12.h"
#include "Include/ImGui/imgui_impl_win32.h"
#include "Include/ImGui/imgui.h"
#endif

// ------------------------------------------------ Window Proc ------------------------------------------------

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TerrainApp* app = reinterpret_cast<TerrainApp*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));

	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
		return 1;

	switch (msg)
	{
	case WM_SIZE:
	{
		int width = LOWORD(lParam);
		int height = HIWORD(lParam);

		if (app)
			app->OnWindowResize(width, height);

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

	// ------------------------------------------------ Window Creation ------------------------------------------------

	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpszClassName = L"TerrainAppWindowClass";
	wc.lpfnWndProc = &WndProc;

	if (!::RegisterClassExW(&wc))
	{
		std::cerr << "RegisterClassExW failed (" << ::GetLastError() << ")\n";
		return;
	}

	int width = m_width;
	int height = m_height;

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

	if (m_maximizeAtStart) 
	{
		RECT workArea;
		MONITORINFO monitorInfo;
		monitorInfo.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST), &monitorInfo);
		workArea = monitorInfo.rcWork;

		SetWindowPos(m_hwnd, HWND_TOP, workArea.left, workArea.top, workArea.right - workArea.left, workArea.bottom - workArea.top, SWP_SHOWWINDOW);

		RECT ClientRect;
		GetClientRect(m_hwnd, &ClientRect);
		width = ClientRect.right - ClientRect.left;
		height = ClientRect.bottom - ClientRect.top;
	}

	::ShowCursor(false);
	int centerX = width / 2;
	int centerY = height / 2;
	::SetCursorPos(centerX, centerY);

	// ------------------------------------------------ D3D12 Device Creation ------------------------------------------------

	HRESULT hr;

	IDXGIFactory4* dxgiFactory;
	hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));

	if (FAILED(hr))
	{
		return;
	}

	IDXGIAdapter1* adapter;
	int adapterIndex = 0;
	bool adapterFound = false;

	while (dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			adapterIndex++;
			continue;
		}

		hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);

		if (SUCCEEDED(hr))
		{
			adapterFound = true;
			break;
		}

		adapterIndex++;
	}

	if (!adapterFound)
	{
		return;
	}

	hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));

	if (FAILED(hr))
	{
		return;
	}

	DXGI_ADAPTER_DESC Desc;
	adapter->GetDesc(&Desc);
	std::wstring WideName = Desc.Description;
	std::string DeviceName = std::string(WideName.begin(), WideName.end());

	std::cout << "D3D12 Device : " << DeviceName << std::endl;

	// ------------------------------------------------ D3D12 Init (Cmd Queue, Swap Chain) ------------------------------------------------

	D3D12_COMMAND_QUEUE_DESC cqDesc = {};

	hr = m_device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&m_commandQueue));

	if (FAILED(hr))
	{
		return;
	}

	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc = sampleDesc;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = FRAMES_IN_FLIGHT;
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;

	IDXGISwapChain1* tempSwapChain;

	hr = dxgiFactory->CreateSwapChainForHwnd(m_commandQueue, m_hwnd, &swapChainDesc, nullptr, nullptr, &tempSwapChain);

	if (FAILED(hr))
	{
		return;
	}

	tempSwapChain->QueryInterface(IID_PPV_ARGS(&m_swapChain));
	tempSwapChain->Release();

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// ------------------------------------------------ D3D12 Init (CBV SRV DSV Descriptor Heaps) ------------------------------------------------

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = FRAMES_IN_FLIGHT + 1; // 1 for each backbuffer, and 1 to render to texture
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvDescriptorHeap));

	if (FAILED(hr))
	{
		return;
	}

	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));

		if (FAILED(hr))
		{
			return;
		}

		m_device->CreateRenderTargetView(m_renderTargets[i], nullptr, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsDescriptorHeap));

	if (FAILED(hr))
	{
		return;
	}

	CD3DX12_HEAP_PROPERTIES heapPropDefault(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	CD3DX12_RESOURCE_DESC depthStencilDescRes = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_D32_FLOAT,
		width,
		height,
		1,
		0,
		1,
		0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	);

	m_device->CreateCommittedResource(
		&heapPropDefault,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDescRes,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&m_depthStencilBuffer)
	);

	m_dsDescriptorHeap->SetName(L"Depth/Stencil Resource Heap");

	m_device->CreateDepthStencilView(m_depthStencilBuffer, &depthStencilDesc, m_dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 100;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		hr = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_mainDescriptorHeap[i]));

		if (FAILED(hr))
		{
			return;
		}
	}

	for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
	{
		CD3DX12_HEAP_PROPERTIES heapPropUpload(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(1024 * 64);

		hr = m_device->CreateCommittedResource(
			&heapPropUpload,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_constantBufferUploadHeap[i]));

		m_constantBufferUploadHeap[i]->SetName(L"Constant Buffer Upload Resource Heap");

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_constantBufferUploadHeap[i]->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = (sizeof(ConstantBuffer) + 255) & ~255;
		m_device->CreateConstantBufferView(&cbvDesc, m_mainDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart()); // Constant Buffer (b0)

		ZeroMemory(&m_constantBuffer, sizeof(m_constantBuffer));

		CD3DX12_RANGE readRange(0, 0);
		hr = m_constantBufferUploadHeap[i]->Map(0, &readRange, reinterpret_cast<void**>(&m_constantBufferGPUAddress[i]));
		memcpy(m_constantBufferGPUAddress[i], &m_constantBuffer, sizeof(m_constantBuffer));
	}

	// ------------------------------------------------ D3D12 Init (Cmd List, Fences) ------------------------------------------------

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i]));

		if (FAILED(hr))
		{
			return;
		}
	}

	hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[0], NULL, IID_PPV_ARGS(&m_commandList));

	if (FAILED(hr))
	{
		return;
	}

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fences[i]));

		if (FAILED(hr))
		{
			return;
		}

		m_fenceValues[i] = 0;
	}

	m_fenceEvent = CreateEvent(nullptr, false, false, nullptr);

	if (m_fenceEvent == nullptr)
	{
		return;
	}

	// ------------------------------------------------ D3D12 Init (Root Signature) ------------------------------------------------

	// Descriptor 0: Constant Buffer (b0)
	// Descriptor 1: Albedo Texture (t0)
	// Descriptor 2: Baked Heightmap (t1)

	D3D12_DESCRIPTOR_RANGE  descriptorTableRanges[2];

	descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descriptorTableRanges[0].NumDescriptors = 1; // b0
	descriptorTableRanges[0].BaseShaderRegister = 0;
	descriptorTableRanges[0].RegisterSpace = 0;
	descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	descriptorTableRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorTableRanges[1].NumDescriptors = 2; // t0-t1
	descriptorTableRanges[1].BaseShaderRegister = 0;
	descriptorTableRanges[1].RegisterSpace = 0;
	descriptorTableRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
	descriptorTable.NumDescriptorRanges = _countof(descriptorTableRanges);
	descriptorTable.pDescriptorRanges = &descriptorTableRanges[0];

	D3D12_ROOT_PARAMETER  rootParameters[1];
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].DescriptorTable = descriptorTable;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ID3DBlob* signature;
	hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);

	if (FAILED(hr))
	{
		return;
	}

	hr = m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));

	if (FAILED(hr))
	{
		return;
	}

	// ------------------------------------------------ Shaders Compilation ------------------------------------------------

	ShaderData vertexShaderData = {};
	ShaderCompiler::CompileShaderFromFile("Shaders/Vertex.hlsl", ShaderType::Vertex, vertexShaderData);

	D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
	vertexShaderBytecode.BytecodeLength = vertexShaderData.bytecode.size() * sizeof(uint32_t);
	vertexShaderBytecode.pShaderBytecode = vertexShaderData.bytecode.data();


	ShaderData pixelShaderData = {};
	ShaderCompiler::CompileShaderFromFile("Shaders/Pixel.hlsl", ShaderType::Pixel, pixelShaderData);

	D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
	pixelShaderBytecode.BytecodeLength = pixelShaderData.bytecode.size() * sizeof(uint32_t);
	pixelShaderBytecode.pShaderBytecode = pixelShaderData.bytecode.data();


	ShaderData hullShaderData = {};
	ShaderCompiler::CompileShaderFromFile("Shaders/Hull.hlsl", ShaderType::Hull, hullShaderData);

	D3D12_SHADER_BYTECODE hullShaderBytecode = {};
	hullShaderBytecode.BytecodeLength = hullShaderData.bytecode.size() * sizeof(uint32_t);
	hullShaderBytecode.pShaderBytecode = hullShaderData.bytecode.data();


	ShaderData domainShaderData = {};
	ShaderCompiler::CompileShaderFromFile("Shaders/Domain.hlsl", ShaderType::Domain, domainShaderData);

	D3D12_SHADER_BYTECODE domainShaderBytecode = {};
	domainShaderBytecode.BytecodeLength = domainShaderData.bytecode.size() * sizeof(uint32_t);
	domainShaderBytecode.pShaderBytecode = domainShaderData.bytecode.data();

	// VT Shaders

	ShaderData pixelVTShaderData = {};
	ShaderCompiler::CompileShaderFromFile("Shaders/PixelVT.hlsl", ShaderType::Pixel, pixelVTShaderData);

	D3D12_SHADER_BYTECODE pixelVTShaderBytecode = {};
	pixelVTShaderBytecode.BytecodeLength = pixelVTShaderData.bytecode.size() * sizeof(uint32_t);
	pixelVTShaderBytecode.pShaderBytecode = pixelVTShaderData.bytecode.data();

	// ------------------------------------------------ D3D12 Init (PSOs) ------------------------------------------------

	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};

	inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	inputLayoutDesc.pInputElementDescs = inputLayout;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.pRootSignature = m_rootSignature;
	psoDesc.VS = vertexShaderBytecode;
	psoDesc.PS = pixelShaderBytecode;
	psoDesc.HS = hullShaderBytecode;
	psoDesc.DS = domainShaderBytecode;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc = sampleDesc;
	psoDesc.SampleMask = 0xffffffff;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FillMode = m_drawWireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.NumRenderTargets = 1;
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateObject));

	if (FAILED(hr))
	{
		return;
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC rtPsoDesc = {};
	rtPsoDesc.InputLayout = inputLayoutDesc;
	rtPsoDesc.pRootSignature = m_rootSignature;
	rtPsoDesc.VS = vertexShaderBytecode;
	rtPsoDesc.PS = pixelVTShaderBytecode; // Change only PS for VT page query
	rtPsoDesc.HS = hullShaderBytecode;
	rtPsoDesc.DS = domainShaderBytecode;
	rtPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	rtPsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtPsoDesc.SampleDesc = sampleDesc;
	rtPsoDesc.SampleMask = 0xffffffff;
	rtPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rtPsoDesc.RasterizerState.FillMode = m_drawWireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
	rtPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	rtPsoDesc.NumRenderTargets = 1;
	rtPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	hr = m_device->CreateGraphicsPipelineState(&rtPsoDesc, IID_PPV_ARGS(&m_renderToTexturePSO));

	if (FAILED(hr))
	{
		return;
	}

	// ------------------------------------------------ D3D12 Init (Compute Pipeline) ------------------------------------------------

	// Root Parameter 0: Descriptor 0: Constant Buffer (b0)
	// Root Parameter 1: Descriptor 4: Compute Heightmap UAV (u0)

	D3D12_DESCRIPTOR_RANGE computeCBVRange;
	computeCBVRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	computeCBVRange.NumDescriptors = 1; // b0
	computeCBVRange.BaseShaderRegister = 0;
	computeCBVRange.RegisterSpace = 0;
	computeCBVRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE computeUAVRange;
	computeUAVRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	computeUAVRange.NumDescriptors = 1; // u0
	computeUAVRange.BaseShaderRegister = 0;
	computeUAVRange.RegisterSpace = 0;
	computeUAVRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_DESCRIPTOR_TABLE computeCBVTable;
	computeCBVTable.NumDescriptorRanges = 1;
	computeCBVTable.pDescriptorRanges = &computeCBVRange;

	D3D12_ROOT_DESCRIPTOR_TABLE computeUAVTable;
	computeUAVTable.NumDescriptorRanges = 1;
	computeUAVTable.pDescriptorRanges = &computeUAVRange;

	D3D12_ROOT_PARAMETER computeRootParameters[2];
	computeRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	computeRootParameters[0].DescriptorTable = computeCBVTable;
	computeRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	computeRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	computeRootParameters[1].DescriptorTable = computeUAVTable;
	computeRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	CD3DX12_ROOT_SIGNATURE_DESC computeRootSignatureDesc;
	computeRootSignatureDesc.Init(_countof(computeRootParameters), computeRootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

	ID3DBlob* computeSignature;
	hr = D3D12SerializeRootSignature(&computeRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &computeSignature, nullptr);

	if (FAILED(hr))
	{
		return;
	}

	hr = m_device->CreateRootSignature(0, computeSignature->GetBufferPointer(), computeSignature->GetBufferSize(), IID_PPV_ARGS(&m_computeRootSignature));

	if (FAILED(hr))
	{
		return;
	}

	ShaderData computeShaderData = {};
	ShaderCompiler::CompileShaderFromFile("Shaders/HeightMapBakingCompute.hlsl", ShaderType::Compute, computeShaderData);

	D3D12_SHADER_BYTECODE computeShaderBytecode = {};
	computeShaderBytecode.BytecodeLength = computeShaderData.bytecode.size() * sizeof(uint32_t);
	computeShaderBytecode.pShaderBytecode = computeShaderData.bytecode.data();

	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
	computePsoDesc.pRootSignature = m_computeRootSignature;
	computePsoDesc.CS = computeShaderBytecode;
	computePsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	 hr = m_device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&m_computePipelineState));
	
	 if (FAILED(hr))
	 {
	 	return;
	 }

	// ------------------------------------------------ Terrain Base Mesh Generation ------------------------------------------------

	/*const float PLANE_SIZE = 1000.0f;
	const float QUAD_SIZE = 20.0f;*/

	const float PLANE_SIZE = 10.0f;
	const float QUAD_SIZE = 2.0f;
	const int GRID_DIVISIONS = static_cast<int>(PLANE_SIZE / QUAD_SIZE);

	std::vector<Vertex> vList;
	for (int z = 0; z <= GRID_DIVISIONS; ++z) 
	{
		for (int x = 0; x <= GRID_DIVISIONS; ++x) 
		{
			float xPos = -PLANE_SIZE / 2.0f + x * QUAD_SIZE;
			float zPos = -PLANE_SIZE / 2.0f + z * QUAD_SIZE;

			float u = static_cast<float>(x) / GRID_DIVISIONS;
			float v = static_cast<float>(z) / GRID_DIVISIONS;

			vList.push_back({ { xPos, 0.0f, zPos }, { u, v } });
		}
	}

	std::vector<DWORD> iList;
	for (int z = 0; z < GRID_DIVISIONS; ++z) 
	{
		for (int x = 0; x < GRID_DIVISIONS; ++x) 
		{
			int topLeft = z * (GRID_DIVISIONS + 1) + x;
			int topRight = topLeft + 1;
			int bottomLeft = (z + 1) * (GRID_DIVISIONS + 1) + x;
			int bottomRight = bottomLeft + 1;

			iList.push_back(topLeft);
			iList.push_back(bottomRight);
			iList.push_back(topRight);

			iList.push_back(topLeft);
			iList.push_back(bottomLeft);
			iList.push_back(bottomRight);
		}
	}

	m_vertexCount = static_cast<UINT>(vList.size());
	m_indexCount = static_cast<UINT>(iList.size());

	// ------------------------------------------------ Terrain Vertex/Index Buffers ------------------------------------------------

	int vBufferSize = sizeof(Vertex) * vList.size();

	CD3DX12_RESOURCE_DESC descBufferSize = CD3DX12_RESOURCE_DESC::Buffer(vBufferSize);

	m_device->CreateCommittedResource(
		&heapPropDefault,
		D3D12_HEAP_FLAG_NONE,
		&descBufferSize,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer));

	m_vertexBuffer->SetName(L"Vertex Buffer Resource Heap");

	CD3DX12_HEAP_PROPERTIES heapPropUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	ID3D12Resource* vBufferUploadHeap;
	m_device->CreateCommittedResource(
		&heapPropUpload,
		D3D12_HEAP_FLAG_NONE,
		&descBufferSize,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vBufferUploadHeap));

	vBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

	D3D12_SUBRESOURCE_DATA vertexData = {};
	vertexData.pData = reinterpret_cast<BYTE*>(vList.data());
	vertexData.RowPitch = vBufferSize;
	vertexData.SlicePitch = vBufferSize;

	UpdateSubresources(m_commandList, m_vertexBuffer, vBufferUploadHeap, 0, 0, 1, &vertexData);

	CD3DX12_RESOURCE_BARRIER copyToVsBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	m_commandList->ResourceBarrier(1, &copyToVsBarrier);

	int iBufferSize = sizeof(DWORD) * iList.size();

	CD3DX12_RESOURCE_DESC descIndexBufferSize = CD3DX12_RESOURCE_DESC::Buffer(iBufferSize);

	m_device->CreateCommittedResource(
		&heapPropDefault,
		D3D12_HEAP_FLAG_NONE,
		&descIndexBufferSize,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_indexBuffer));

	m_vertexBuffer->SetName(L"Index Buffer Resource Heap");

	ID3D12Resource* iBufferUploadHeap;
	m_device->CreateCommittedResource(
		&heapPropUpload,
		D3D12_HEAP_FLAG_NONE,
		&descIndexBufferSize,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&iBufferUploadHeap));

	vBufferUploadHeap->SetName(L"Index Buffer Upload Resource Heap");

	D3D12_SUBRESOURCE_DATA indexData = {};
	indexData.pData = reinterpret_cast<BYTE*>(iList.data());
	indexData.RowPitch = iBufferSize;
	indexData.SlicePitch = iBufferSize;

	UpdateSubresources(m_commandList, m_indexBuffer, iBufferUploadHeap, 0, 0, 1, &indexData);

	m_commandList->ResourceBarrier(1, &copyToVsBarrier);

	// ------------------------------------------------ Terrain Test Texture ------------------------------------------------

	m_albedoTexture.LoadFromFile(m_device, m_commandList, "Assets/rocks_albedo.png");

	UINT descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(
			m_mainDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart(),
			1, // Albedo Texture (t0)
			descriptorSize);

		m_albedoTexture.CreateSRV(m_device, srvHandle);
	}

	// ------------------------------------------------ Baked Heightmap Loading ------------------------------------------------

	m_bakedHeightmapTexture.LoadFromFile(m_device, m_commandList, "Assets/HeightMap.png", DXGI_FORMAT_R32_FLOAT);

	for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE bakedHeightmapSrvHandle(
			m_mainDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart(),
			2, // Baked Heightmap (t1)
			descriptorSize);

		m_bakedHeightmapTexture.CreateSRV(m_device, bakedHeightmapSrvHandle);
	}

	// ------------------------------------------------ Compute Shader Resources ------------------------------------------------

	m_computeOutputTexture.CreateEmpty(
		m_device,
		1080,
		1080,
		DXGI_FORMAT_R32_FLOAT,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(
			m_mainDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart(),
			4, // Compute Heightmap UAV (u0)
			descriptorSize);

		m_computeOutputTexture.CreateUAV(m_device, uavHandle);

		CD3DX12_CPU_DESCRIPTOR_HANDLE computeSrvHandle(
			m_mainDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart(),
			5, // Compute Heightmap SRV (used by ImGui only)
			descriptorSize);

		m_computeOutputTexture.CreateSRV(m_device, computeSrvHandle);
	}

	UINT64 readbackBufferSize = 1080 * 1080 * 4;
	CD3DX12_HEAP_PROPERTIES readbackHeapProps(D3D12_HEAP_TYPE_READBACK);
	CD3DX12_RESOURCE_DESC readbackBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(readbackBufferSize);

	hr = m_device->CreateCommittedResource(
		&readbackHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&readbackBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_heightmapReadbackBuffer));

	if (FAILED(hr))
	{
		return;
	}

	m_heightmapReadbackBuffer->SetName(L"Heightmap Readback Buffer");

	CD3DX12_RESOURCE_BARRIER barriers[1];
	barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
		m_computeOutputTexture.resource,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	m_commandList->ResourceBarrier(1, barriers);

	// ------------------------------------------------ Render To Texture Init ------------------------------------------------

	m_renderTexture.CreateEmpty(
		m_device,
		width,
		height,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	m_renderTexture.CreateRTV(m_device, rtvHandle /* after frames in flight */);

	for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE renderTextureSrvHandle(
			m_mainDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart(),
			6, // Render To Texture SRV (ImGui use only atm)
			descriptorSize);

		m_renderTexture.CreateSRV(m_device, renderTextureSrvHandle);
	}

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	UINT64 RTreadbackBufferSize;
	m_renderTexture.GetFootprint(m_device, footprint, RTreadbackBufferSize);

	CD3DX12_RESOURCE_DESC RTreadbackBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(RTreadbackBufferSize);

	hr = m_device->CreateCommittedResource(
		&readbackHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&RTreadbackBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_VTpagesRequestReadBackBuffer));

	if (FAILED(hr))
	{
		return;
	}

	m_VTpagesRequestReadBackBuffer->SetName(L"VT Page Request Readback Buffer");

	CD3DX12_RESOURCE_BARRIER RTbarriers[1];

	RTbarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
		m_renderTexture.resource,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	m_commandList->ResourceBarrier(1, RTbarriers);

	// ------------------------------------------------ VT Main Memory Texture ------------------------------------------------

	m_VTMainMemoryTexture.CreateEmpty(
		m_device,
		16,
		16,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE renderTextureSrvHandle(
			m_mainDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart(),
			7, // VT Main Memory Texture (ImGui use only atm)
			descriptorSize);

		m_VTMainMemoryTexture.CreateSRV(m_device, renderTextureSrvHandle);
	}

	CD3DX12_RESOURCE_BARRIER VTtoSrvBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_VTMainMemoryTexture.resource,
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// ------------------------------------------------ D3D12 Init Execute Commands ------------------------------------------------

	m_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_commandList };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	m_fenceValues[m_frameIndex]++;
	hr = m_commandQueue->Signal(m_fences[m_frameIndex], m_fenceValues[m_frameIndex]);

	if (FAILED(hr))
	{
		return;
	}

	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_indexBufferView.SizeInBytes = iBufferSize;

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vBufferSize;

	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
	m_viewport.Width = width;
	m_viewport.Height = height;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = width;
	m_scissorRect.bottom = height;

	// ------------------------------------------------ App Init Data ------------------------------------------------

	m_camera.SetPosition({ 0.0f, 6.0f, -5.0f });
	m_camera.UpdatePerspectiveFOV(0.35f * 3.14159f, (float)width / (float)height);

	m_constantBuffer.noise_persistence = 0.38f;
	m_constantBuffer.noise_lacunarity = 2.6f;
	m_constantBuffer.noise_scale = 3.0f;
	m_constantBuffer.noise_height = 110.0f;
	m_constantBuffer.noise_octaves = 5;
	m_constantBuffer.runtime_noise = m_runtimeNoiseAtStart;

#if ENABLE_IMGUI

	// ------------------------------------------------ ImGui Init ------------------------------------------------

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_EnableDpiAwareness();
	ImGui_ImplWin32_Init(m_hwnd);
	ImGui_ImplDX12_Init(m_device,
		FRAMES_IN_FLIGHT,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		m_mainDescriptorHeap[0],
		CD3DX12_CPU_DESCRIPTOR_HANDLE(m_mainDescriptorHeap[0]->GetCPUDescriptorHandleForHeapStart(), IMGUI_DESCRIPTOR_OFFSET, descriptorSize), // Descriptor 20+: ImGui Font Atlas
		CD3DX12_GPU_DESCRIPTOR_HANDLE(m_mainDescriptorHeap[0]->GetGPUDescriptorHandleForHeapStart(), IMGUI_DESCRIPTOR_OFFSET, descriptorSize));

#endif

	m_isRunning = true;
}

TerrainApp::~TerrainApp()
{
	for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
	{
		const UINT64 fenceToWaitFor = m_fenceValues[i];
		m_commandQueue->Signal(m_fences[i], fenceToWaitFor);

		if (m_fences[i]->GetCompletedValue() < fenceToWaitFor)
		{
			m_fences[i]->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent);
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}
	}

#if ENABLE_IMGUI

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

#endif

	if (m_device)
		m_device->Release();

	if (m_swapChain)
		m_swapChain->Release();

	if (m_commandQueue)
		m_commandQueue->Release();

	if (m_rtvDescriptorHeap)
		m_rtvDescriptorHeap->Release();

	if (m_commandList)
		m_commandList->Release();

	for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
	{
		if (m_mainDescriptorHeap[i])
			m_mainDescriptorHeap[i]->Release();

		if(m_constantBufferUploadHeap[i])
			m_constantBufferUploadHeap[i]->Release();

		if (m_renderTargets[i])
			m_renderTargets[i]->Release();

		if (m_commandAllocators[i])
			m_commandAllocators[i]->Release();

		if (m_fences[i])
			m_fences[i]->Release();
	};

	m_albedoTexture.Release();
	m_computeOutputTexture.Release();
	m_bakedHeightmapTexture.Release();
	m_renderTexture.Release();
	m_VTMainMemoryTexture.Release();

	if (m_heightmapReadbackBuffer)
		m_heightmapReadbackBuffer->Release();

	if (m_VTpagesRequestReadBackBuffer)
		m_VTpagesRequestReadBackBuffer->Release();

	if (m_computePipelineState)
		m_computePipelineState->Release();

	if (m_computeRootSignature)
		m_computeRootSignature->Release();

	::DestroyWindow(m_hwnd);
}

void TerrainApp::Run()
{
	while (m_isRunning)
	{
		// ------------------------------------------------ App Update ------------------------------------------------

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

		m_camera.Walk(m_cameraForward * dt * m_cameraMoveSpeed);
		m_camera.Strafe(m_cameraRight * dt * m_cameraMoveSpeed);

		m_camera.UpdateViewMatrix();

		WaitForPreviousFrame();

		DirectX::XMMATRIX view = m_camera.GetViewMatrix();
		DirectX::XMMATRIX proj = m_camera.GetProjMatrix();
		DirectX::XMMATRIX viewProj = view * proj;

		DirectX::XMStoreFloat4x4(&m_constantBuffer.viewProj, viewProj);

		memcpy(m_constantBufferGPUAddress[m_frameIndex], &m_constantBuffer, sizeof(m_constantBuffer));

		// ------------------------------------------------ D3D12 Commands ------------------------------------------------

		HRESULT hr;

		hr = m_commandAllocators[m_frameIndex]->Reset();

		if (FAILED(hr))
		{
			return;
		}

		hr = m_commandList->Reset(m_commandAllocators[m_frameIndex], m_pipelineStateObject);

		if (FAILED(hr))
		{
			return;
		}

		// Render to texture

		CD3DX12_RESOURCE_BARRIER srvToRtTransition = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTexture.resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

		m_commandList->ResourceBarrier(1, &srvToRtTransition);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtHandle(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), FRAMES_IN_FLIGHT, m_rtvDescriptorSize);
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		m_commandList->OMSetRenderTargets(1, &rtHandle, false, &dsvHandle);

		const float rtClearColor[] = { 0.f, 0.0f, 0.0f, 0.0f /* 0 indicates no page requested here */};
		m_commandList->ClearRenderTargetView(rtHandle, rtClearColor, 0, nullptr);
		m_commandList->ClearDepthStencilView(m_dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		m_commandList->SetPipelineState(m_renderToTexturePSO);

		m_commandList->SetGraphicsRootSignature(m_rootSignature);

		ID3D12DescriptorHeap* descriptorHeaps[] = { m_mainDescriptorHeap[m_frameIndex] };
		m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		m_commandList->SetGraphicsRootDescriptorTable(0, m_mainDescriptorHeap[m_frameIndex]->GetGPUDescriptorHandleForHeapStart());

		m_commandList->RSSetViewports(1, &m_viewport);
		m_commandList->RSSetScissorRects(1, &m_scissorRect);
		m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
		m_commandList->IASetIndexBuffer(&m_indexBufferView);
		m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		m_commandList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);

		CD3DX12_RESOURCE_BARRIER rtToSrvTransition = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTexture.resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_commandList->ResourceBarrier(1, &rtToSrvTransition);

		// Render to backbuffer

		CD3DX12_RESOURCE_BARRIER presentToRtTransition = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		m_commandList->ResourceBarrier(1, &presentToRtTransition);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);

		m_commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

		const float clearColor[] = { 0.0f, 0.0f, 0.1f, 0.0f };
		m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		m_commandList->ClearDepthStencilView(m_dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		m_commandList->SetPipelineState(m_pipelineStateObject);

		m_commandList->SetGraphicsRootSignature(m_rootSignature);

		m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		m_commandList->SetGraphicsRootDescriptorTable(0, m_mainDescriptorHeap[m_frameIndex]->GetGPUDescriptorHandleForHeapStart());

		m_commandList->RSSetViewports(1, &m_viewport);
		m_commandList->RSSetScissorRects(1, &m_scissorRect);
		m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
		m_commandList->IASetIndexBuffer(&m_indexBufferView);
		m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		m_commandList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);

#if ENABLE_IMGUI

		m_commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

		// ------------------------------------------------ ImGui Commands ------------------------------------------------

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("FrameRate");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::SliderFloat("Move Speed", &m_cameraMoveSpeed, 1.0f, 100.0f);
		ImGui::Text("Pre-tesselation Vertex Count: %d", m_vertexCount);
		const int tessFactor = 64;
		int baseTriangles = m_vertexCount / 3;
		int tessVertexCount = baseTriangles * ((tessFactor + 1) * (tessFactor + 2) / 2);
		ImGui::Text("Tessellated Vertex Count: %d", tessVertexCount);
		ImGui::Checkbox("Runtime Perlin Noise", &m_constantBuffer.runtime_noise);
		ImGui::End();

		UINT descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		ImGui::Begin("Render To Texture");

		if (ImGui::Button("Readback VT Page Requests")) 
		{
			CD3DX12_RESOURCE_BARRIER srvToCopyBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
				m_renderTexture.resource,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_COPY_SOURCE);
			m_commandList->ResourceBarrier(1, &srvToCopyBarrier);

			D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
			UINT64 RTreadbackBufferSize;
			m_renderTexture.GetFootprint(m_device, footprint, RTreadbackBufferSize);

			D3D12_TEXTURE_COPY_LOCATION rtSrcLocation = {};
			rtSrcLocation.pResource = m_renderTexture.resource;
			rtSrcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			rtSrcLocation.SubresourceIndex = 0;

			D3D12_TEXTURE_COPY_LOCATION rtDstLocation = {};
			rtDstLocation.pResource = m_VTpagesRequestReadBackBuffer;
			rtDstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			rtDstLocation.PlacedFootprint.Offset = 0;
			rtDstLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			rtDstLocation.PlacedFootprint.Footprint.Width = m_width;
			rtDstLocation.PlacedFootprint.Footprint.Height = m_height;
			rtDstLocation.PlacedFootprint.Footprint.Depth = 1;
			rtDstLocation.PlacedFootprint.Footprint.RowPitch = footprint.Footprint.RowPitch;

			m_commandList->CopyTextureRegion(&rtDstLocation, 0, 0, 0, &rtSrcLocation, nullptr);

			CD3DX12_RESOURCE_BARRIER copyToSrvBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
				m_renderTexture.resource,
				D3D12_RESOURCE_STATE_COPY_SOURCE,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			m_commandList->ResourceBarrier(1, &copyToSrvBarrier);

			m_checkVTpagesRequestAfterFrame = true;
		}

		CD3DX12_GPU_DESCRIPTOR_HANDLE rtSrvGpuHandle(
			m_mainDescriptorHeap[m_frameIndex]->GetGPUDescriptorHandleForHeapStart(),
			6, // Render Texture SRV
			descriptorSize);

		ImGui::Image((ImTextureID)rtSrvGpuHandle.ptr, ImVec2(m_width / 2, m_height / 2));

		ImGui::End();

		ImGui::Begin("VT Main Memory Texture");

		if (ImGui::Button("Load VT Pages In VRAM"))
		{
			for (int i = 0; i < 100; i++) // TODO remove this please
			{
				if (m_VTpagesRequestResult.uploadHeaps[i])
					m_VTpagesRequestResult.uploadHeaps[i]->Release();
			}

			VTexHeader vTexHeader;
			if (VTex::LoadHeader("Assets/TestVT.vtex", vTexHeader)) 
			{

				CD3DX12_RESOURCE_BARRIER srvToCopyBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
					m_VTMainMemoryTexture.resource,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_COPY_DEST);
				m_commandList->ResourceBarrier(1, &srvToCopyBarrier);

				int i = 0;
				for (auto it = m_VTpagesRequestResult.requestedPages.begin(); it != m_VTpagesRequestResult.requestedPages.end(); ++it, ++i) 
				{
					auto coords = *it;

					bool alreadyLoadedPage = false;
					for (const auto& loadedPage : m_VTpagesRequestResult.loadedPages) 
					{
						if (loadedPage.coords == coords) // page already loaded with same coords
						{
							std::cout << "VT Page request but page is already loaded : " << coords.first << ", " << coords.second << ")" << std::endl;
							alreadyLoadedPage = true;
							break;
						}
					}

					if (alreadyLoadedPage)
						continue;

					std::vector<char> texTileData;
					if (VTex::LoadTile("Assets/TestVT.vtex", coords.first, coords.second, texTileData))
					{
						D3D12_RESOURCE_DESC textureDesc = {};
						textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
						textureDesc.Alignment = 0;
						textureDesc.Width = vTexHeader.tileSize;
						textureDesc.Height = vTexHeader.tileSize;
						textureDesc.DepthOrArraySize = 1;
						textureDesc.MipLevels = 1;
						textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
						textureDesc.SampleDesc.Count = 1;
						textureDesc.SampleDesc.Quality = 0;
						textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
						textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

						UINT64 uploadBufferSize;
						m_device->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

						CD3DX12_HEAP_PROPERTIES heapPropUpload(D3D12_HEAP_TYPE_UPLOAD);
						CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

						ID3D12Resource* uploadHeap;

						hr = m_device->CreateCommittedResource(
							&heapPropUpload,
							D3D12_HEAP_FLAG_NONE,
							&uploadBufferDesc,
							D3D12_RESOURCE_STATE_GENERIC_READ,
							nullptr,
							IID_PPV_ARGS(&m_VTpagesRequestResult.uploadHeaps[i]));

						D3D12_SUBRESOURCE_FOOTPRINT footprint = {};
						footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
						footprint.Width = vTexHeader.tileSize;
						footprint.Height = vTexHeader.tileSize;
						footprint.Depth = 1;
						footprint.RowPitch = (vTexHeader.tileSize * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1)
							& ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);

						UINT numLoadedTiles = static_cast<UINT>(m_VTpagesRequestResult.loadedPages.size());
						UINT tilesPerRow = m_VTMainMemoryTexture.width / vTexHeader.tileSize;

						UINT tileIndexX = numLoadedTiles % tilesPerRow;
						UINT tileIndexY = numLoadedTiles / tilesPerRow;

						UINT tileX = tileIndexX * vTexHeader.tileSize;
						UINT tileY = tileIndexY * vTexHeader.tileSize;

						UINT8* mappedData = nullptr;
						CD3DX12_RANGE readRange(0, 0);
						m_VTpagesRequestResult.uploadHeaps[i]->Map(0, &readRange, reinterpret_cast<void**>(&mappedData));

						const UINT srcRowPitch = vTexHeader.tileSize * 4;
						const UINT8* srcData = reinterpret_cast<const UINT8*>(texTileData.data());
						for (UINT row = 0; row < vTexHeader.tileSize; ++row)
						{
							memcpy(
								mappedData + footprint.RowPitch * row,
								srcData + srcRowPitch * row,
								srcRowPitch);
						}

						m_VTpagesRequestResult.uploadHeaps[i]->Unmap(0, nullptr);

						D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
						srcLocation.pResource = m_VTpagesRequestResult.uploadHeaps[i];
						srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
						srcLocation.PlacedFootprint.Footprint = footprint;

						D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
						dstLocation.pResource = m_VTMainMemoryTexture.resource;
						dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
						dstLocation.SubresourceIndex = 0;

						CD3DX12_BOX srcBox(0, 0, 0, vTexHeader.tileSize, vTexHeader.tileSize, 1);

						m_commandList->CopyTextureRegion(&dstLocation, tileX, tileY, 0, &srcLocation, &srcBox);

						VTPage loadedVTPage;
						loadedVTPage.coords = { coords.first, coords.second };
						m_VTpagesRequestResult.loadedPages.insert(loadedVTPage);
					}
				}

				CD3DX12_RESOURCE_BARRIER copyToSrvBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
					m_VTMainMemoryTexture.resource,
					D3D12_RESOURCE_STATE_COPY_DEST,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				m_commandList->ResourceBarrier(1, &copyToSrvBarrier);
			}
		}

		CD3DX12_GPU_DESCRIPTOR_HANDLE VTSrvGpuHandle(
			m_mainDescriptorHeap[m_frameIndex]->GetGPUDescriptorHandleForHeapStart(),
			7, // VT Main Memory Texture SRV
			descriptorSize);

		ImGui::Image((ImTextureID)VTSrvGpuHandle.ptr, ImVec2(320, 320));

		ImGui::End();

		ImGui::Begin("VTex Converter");

		if (ImGui::Button("Convert TestVT to VTex"))
		{
			VTex::ConvertToVTex("Assets/TestVT.png", 4 /* 4x4 tiles for testing */);
			VTex::ConvertToVTex("../../../../Terrain/Assets/TestVT.png", 4 /* 4x4 tiles for testing */);
		}

		if (ImGui::Button("Test read VTex"))
		{
			VTexHeader vTexHeader;
			VTex::LoadHeader("Assets/TestVT.vtex", vTexHeader);

			std::cout << "Loaded VTex : " << vTexHeader.height << " height, " << vTexHeader.width << " width, " << vTexHeader.bytesPerPixel << " bytes per pixel" << std::endl;
		}

		ImGui::End();

		if (m_constantBuffer.runtime_noise) 
		{
			ImGui::Begin("Perlin Noise Settings");
			ImGui::SliderFloat("Persistence", &m_constantBuffer.noise_persistence, 0.0f, 1.0f);
			ImGui::SliderFloat("Lacunarity", &m_constantBuffer.noise_lacunarity, 1.0f, 6.0f);
			ImGui::SliderFloat("Scale", &m_constantBuffer.noise_scale, 0.1f, 10.0f);
			ImGui::SliderFloat("Height", &m_constantBuffer.noise_height, 1.0f, 300.0f);
			ImGui::SliderInt("Octaves", &m_constantBuffer.noise_octaves, 1, 8);
			ImGui::End();

			ImGui::Begin("Heightmap Baking");

			if (ImGui::Button("Bake Heightmap"))
			{
				CD3DX12_RESOURCE_BARRIER srvToUavBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
					m_computeOutputTexture.resource,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				m_commandList->SetPipelineState(m_computePipelineState);
				m_commandList->SetComputeRootSignature(m_computeRootSignature);

				CD3DX12_GPU_DESCRIPTOR_HANDLE cbvGpuHandle(
					m_mainDescriptorHeap[m_frameIndex]->GetGPUDescriptorHandleForHeapStart(),
					0, // Constant Buffer (b0)
					descriptorSize);
				m_commandList->SetComputeRootDescriptorTable(0, cbvGpuHandle);

				CD3DX12_GPU_DESCRIPTOR_HANDLE uavGpuHandle(
					m_mainDescriptorHeap[m_frameIndex]->GetGPUDescriptorHandleForHeapStart(),
					4, // Compute UAV (u0)
					descriptorSize);
				m_commandList->SetComputeRootDescriptorTable(1, uavGpuHandle);

				m_commandList->Dispatch(135, 135, 1);

				CD3DX12_RESOURCE_BARRIER uavToCopyBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
					m_computeOutputTexture.resource,
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
					D3D12_RESOURCE_STATE_COPY_SOURCE);
				m_commandList->ResourceBarrier(1, &uavToCopyBarrier);

				D3D12_TEXTURE_COPY_LOCATION heightmapSrcLocation = {};
				heightmapSrcLocation.pResource = m_computeOutputTexture.resource;
				heightmapSrcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				heightmapSrcLocation.SubresourceIndex = 0;

				D3D12_TEXTURE_COPY_LOCATION heightmapDstLocation = {};
				heightmapDstLocation.pResource = m_heightmapReadbackBuffer;
				heightmapDstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
				heightmapDstLocation.PlacedFootprint.Offset = 0;
				heightmapDstLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R32_FLOAT;
				heightmapDstLocation.PlacedFootprint.Footprint.Width = 1080;
				heightmapDstLocation.PlacedFootprint.Footprint.Height = 1080;
				heightmapDstLocation.PlacedFootprint.Footprint.Depth = 1;
				heightmapDstLocation.PlacedFootprint.Footprint.RowPitch = 1080 * 4;

				m_commandList->CopyTextureRegion(&heightmapDstLocation, 0, 0, 0, &heightmapSrcLocation, nullptr);

				CD3DX12_RESOURCE_BARRIER bakedTexturesToCopyDestBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
					m_bakedHeightmapTexture.resource,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_COPY_DEST);
				m_commandList->ResourceBarrier(1, &bakedTexturesToCopyDestBarrier);

				m_commandList->CopyResource(m_bakedHeightmapTexture.resource, m_computeOutputTexture.resource);

				CD3DX12_RESOURCE_BARRIER allTexturesToSrvBarriers[2];
				allTexturesToSrvBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
					m_computeOutputTexture.resource,
					D3D12_RESOURCE_STATE_COPY_SOURCE,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				allTexturesToSrvBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
					m_bakedHeightmapTexture.resource,
					D3D12_RESOURCE_STATE_COPY_DEST,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				m_commandList->ResourceBarrier(2, allTexturesToSrvBarriers);

				m_saveHeightmapAfterFrame = true;
			}

			CD3DX12_GPU_DESCRIPTOR_HANDLE heightSrvGpuHandle(
				m_mainDescriptorHeap[m_frameIndex]->GetGPUDescriptorHandleForHeapStart(),
				5, // Compute Heightmap SRV
				descriptorSize);

			ImGui::Image((ImTextureID)heightSrvGpuHandle.ptr, ImVec2(540, 540));
			ImGui::End();
		}

		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList);

#endif

		CD3DX12_RESOURCE_BARRIER rtToPresentTransition = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		m_commandList->ResourceBarrier(1, &rtToPresentTransition);

		hr = m_commandList->Close();

		// ------------------------------------------------ Execute Commands & Present ------------------------------------------------

		if (FAILED(hr))
		{
			return;
		}

		ID3D12CommandList* ppCommandLists[] = { m_commandList };

		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		hr = m_commandQueue->Signal(m_fences[m_frameIndex], m_fenceValues[m_frameIndex]);

		if (FAILED(hr))
		{
			return;
		}

		hr = m_swapChain->Present(false, 0);

		if (FAILED(hr))
		{
			return;
		}

		if (m_saveHeightmapAfterFrame)
		{
			if (m_fences[m_frameIndex]->GetCompletedValue() < m_fenceValues[m_frameIndex])
			{
				m_fences[m_frameIndex]->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent);
				WaitForSingleObject(m_fenceEvent, INFINITE);
			}

			SaveHeightmapToPNG("Assets/HeightMap.png");
			SaveHeightmapToPNG("../../../../Terrain/Assets/HeightMap.png");
			m_saveHeightmapAfterFrame = false;
		}

		if (m_checkVTpagesRequestAfterFrame)
		{
			if (m_fences[m_frameIndex]->GetCompletedValue() < m_fenceValues[m_frameIndex])
			{
				m_fences[m_frameIndex]->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent);
				WaitForSingleObject(m_fenceEvent, INFINITE);
			}
			BuildVTPageRequestResult();
			m_checkVTpagesRequestAfterFrame = false;
		}
	}
}

void TerrainApp::Quit()
{
	m_isRunning = false;
}

void TerrainApp::OnWindowResize(int width, int height)
{
	if (!m_isRunning)
		return;

	for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
	{
		const UINT64 fenceToWaitFor = m_fenceValues[i];
		m_commandQueue->Signal(m_fences[i], fenceToWaitFor);

		if (m_fences[i]->GetCompletedValue() < fenceToWaitFor)
		{
			m_fences[i]->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent);
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}
	}

	for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
	{
		if (m_renderTargets[i]) 
		{
			m_renderTargets[i]->Release();
			m_renderTargets[i] = nullptr;
		}
	};

	if (m_depthStencilBuffer)
	{
		m_depthStencilBuffer->Release();
		m_depthStencilBuffer = nullptr;
	}

	// ------------------------------------------------ Resize RTVs & DSV ------------------------------------------------

	HRESULT hr;

	hr = m_swapChain->ResizeBuffers(
		FRAMES_IN_FLIGHT,
		width,
		height,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		0
	);

	if (FAILED(hr))
	{
		return;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));

		if (FAILED(hr))
		{
			return;
		}

		m_device->CreateRenderTargetView(m_renderTargets[i], nullptr, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}

	CD3DX12_HEAP_PROPERTIES heapPropDefault(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	CD3DX12_RESOURCE_DESC depthStencilDescRes = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_D32_FLOAT,
		width,
		height,
		1,
		0,
		1,
		0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	);

	hr = m_device->CreateCommittedResource(
		&heapPropDefault,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDescRes,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&m_depthStencilBuffer)
	);

	if (FAILED(hr))
	{
		return;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	m_device->CreateDepthStencilView(m_depthStencilBuffer, &depthStencilDesc, m_dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// ------------------------------------------------ Render To Texture Init ------------------------------------------------

	m_renderTexture.Release();

	UINT descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_renderTexture.CreateEmpty(
		m_device,
		width,
		height,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	m_renderTexture.CreateRTV(m_device, rtvHandle /* after frames in flight */);

	for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE renderTextureSrvHandle(
			m_mainDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart(),
			6, // Render To Texture SRV (t2)
			descriptorSize);

		m_renderTexture.CreateSRV(m_device, renderTextureSrvHandle);
	}

	if (m_VTpagesRequestReadBackBuffer)
		m_VTpagesRequestReadBackBuffer->Release();

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	UINT64 RTreadbackBufferSize;
	m_renderTexture.GetFootprint(m_device, footprint, RTreadbackBufferSize);

	CD3DX12_RESOURCE_DESC RTreadbackBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(RTreadbackBufferSize);
	CD3DX12_HEAP_PROPERTIES readbackHeapProps(D3D12_HEAP_TYPE_READBACK);

	hr = m_device->CreateCommittedResource(
		&readbackHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&RTreadbackBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_VTpagesRequestReadBackBuffer));

	if (FAILED(hr))
	{
		return;
	}

	m_VTpagesRequestReadBackBuffer->SetName(L"VT Page Request Readback Buffer");

	// ------------------------------------------------ Resize VP & Update Camera ------------------------------------------------

	m_viewport.Width = static_cast<float>(width);
	m_viewport.Height = static_cast<float>(height);
	m_scissorRect.right = width;
	m_scissorRect.bottom = height;

	m_camera.UpdatePerspectiveFOV(0.35f * 3.14159f, (float)width / (float)height);

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	m_width = width;
	m_height = height;
}

void TerrainApp::WaitForPreviousFrame()
{
	HRESULT hr;

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	if (m_fences[m_frameIndex]->GetCompletedValue() < m_fenceValues[m_frameIndex])
	{
		hr = m_fences[m_frameIndex]->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent);

		if (FAILED(hr))
		{
			return;
		}

		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_fenceValues[m_frameIndex]++;
}

// ------------------------------------------------ Input Handling ------------------------------------------------

void TerrainApp::OnMouseMove(Vec2 pos)
{
	if (!m_isMouseLocked)
		return;

	float dx = DirectX::XMConvertToRadians(0.25f * (pos.x - m_lastMousePos.x));
	float dy = DirectX::XMConvertToRadians(0.25f * (pos.y - m_lastMousePos.y));

	m_camera.Pitch(dy);
	m_camera.RotateY(dx);

	m_lastMousePos.x = pos.x;
	m_lastMousePos.y = pos.y;
}

void TerrainApp::OnKeyDown(int key) 
{
	if (key == 'Z')
		m_cameraForward = 1.0f;
	else if (key == 'S')
		m_cameraForward = -1.0f;
	else if (key == 'Q')
		m_cameraRight = -1.0f;
	else if (key == 'D')
		m_cameraRight = 1.0f;
}

void TerrainApp::OnKeyUp(int key)
{
	m_cameraForward = 0.0f;
	m_cameraRight = 0.0f;
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
		m_lastMousePos = Vec2(current_mouse_pos.x, current_mouse_pos.y);
		m_first_time = false;
	}

	if (current_mouse_pos.x != m_lastMousePos.x || current_mouse_pos.y != m_lastMousePos.y)
	{
		OnMouseMove(Vec2(current_mouse_pos.x, current_mouse_pos.y));
	}

	m_lastMousePos = Vec2(current_mouse_pos.x, current_mouse_pos.y);

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

// ------------------------------------------------ Heightmap Saving ------------------------------------------------

void TerrainApp::SaveHeightmapToPNG(const std::string& filepath)
{
	void* pData = nullptr;
	D3D12_RANGE readRange = { 0, 1080 * 1080 * 4 };
	HRESULT hr = m_heightmapReadbackBuffer->Map(0, &readRange, &pData);

	if (FAILED(hr))
	{
		std::cout << "Failed to map readback buffer" << std::endl;
		return;
	}

	float* floatData = static_cast<float*>(pData);
	std::vector<unsigned char> imageData(1080 * 1080);

	for (int i = 0; i < 1080 * 1080; i++)
	{
		float normalized = std::clamp(floatData[i], 0.0f, 1.0f);
		imageData[i] = static_cast<unsigned char>(normalized * 255.0f);
	}

	D3D12_RANGE writeRange = { 0, 0 };
	m_heightmapReadbackBuffer->Unmap(0, &writeRange);

	stbi_flip_vertically_on_write(true);
	if (stbi_write_png(filepath.c_str(), 1080, 1080, 1, imageData.data(), 1080))
	{
		std::cout << "Heightmap saved successfully to " << filepath << std::endl;
	}
	else
	{
		std::cout << "Failed to write heightmap to " << filepath << std::endl;
	}
}

void TerrainApp::BuildVTPageRequestResult()
{
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	UINT64 RTreadbackBufferSize;
	m_renderTexture.GetFootprint(m_device, footprint, RTreadbackBufferSize);

	void* pData = nullptr;
	D3D12_RANGE readRange = { 0, RTreadbackBufferSize };
	HRESULT hr = m_VTpagesRequestReadBackBuffer->Map(0, &readRange, &pData);

	if (FAILED(hr))
	{
		std::cout << "Failed to map readback buffer for VT pages request" << std::endl;
		return;
	}

	int i = 0;

	uint8_t* pByteData = static_cast<uint8_t*>(pData);

	UINT width = footprint.Footprint.Width;
	UINT height = footprint.Footprint.Height;
	UINT rowPitch = footprint.Footprint.RowPitch;

	m_VTpagesRequestResult.requestedPages.clear();

	const int textureSize = 16; // 16x16px tex
	const int pageSize = 4; // 4x4px pages

	for (UINT y = 0; y < height; ++y)
	{
		uint8_t* pRow = pByteData + (y * rowPitch);

		for (UINT x = 0; x < width; ++x)
		{
			uint8_t* pPixel = pRow + (x * 4);

			uint8_t r = pPixel[0];
			uint8_t g = pPixel[1];
			uint8_t b = pPixel[2];
			uint8_t a = pPixel[3];

			if (a > 0) // Alpha channel : something was rendered here and needs a texture page
			{
				int coordX = floor(((float)r / 255.0f) * 16);
				int coordY = floor(((float)g / 255.0f) * 16);

				m_VTpagesRequestResult.requestedPages.insert({ coordX, coordY });
			}

			/*std::cout << "VT Page Request : "
				<< "R: " << static_cast<int>(r) << " "
				<< "G: " << static_cast<int>(g) << " "
				<< "B: " << static_cast<int>(b) << " "
				<< "A: " << static_cast<int>(a) << std::endl;*/
		}
	}

	D3D12_RANGE writeRange = { 0, 0 };
	m_VTpagesRequestReadBackBuffer->Unmap(0, &writeRange);

	for (const auto& coord : m_VTpagesRequestResult.requestedPages) {
		std::cout << "Requested Page Coords : (" << coord.first << ", " << coord.second << ")" << std::endl;
	}

	std::cout << "------------------------------------" << std::endl;
}