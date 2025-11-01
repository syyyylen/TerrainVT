#include "D3D12Renderer.h"

#include <iostream>

D3D12Renderer::D3D12Renderer(HWND hwnd, int width, int height)
{
	std::cout << "Initializing D3D12 Renderer" << std::endl;

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

    D3D12_COMMAND_QUEUE_DESC cqDesc = {};

    hr = m_device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&m_commandQueue));

    if (FAILED(hr))
    {
        return;
    }

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = FRAMES_IN_FLIGHT;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;

    IDXGISwapChain1* tempSwapChain;

    hr = dxgiFactory->CreateSwapChainForHwnd(m_commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, &tempSwapChain);

    if (FAILED(hr))
    {
        return;
	}  

	tempSwapChain->QueryInterface(IID_PPV_ARGS(&m_swapChain));
	tempSwapChain->Release();

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FRAMES_IN_FLIGHT;
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

    m_commandList->Close();

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
}

D3D12Renderer::~D3D12Renderer()
{
    /*for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        m_frameIndex = i;
        WaitForPreviousFrame();
    }*/

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
        if (m_renderTargets[i])
            m_renderTargets[i]->Release();
        
        if (m_commandAllocators[i])
            m_commandAllocators[i]->Release();

        if (m_fences[i])
            m_fences[i]->Release();
    };
}

void D3D12Renderer::Render()
{
    WaitForPreviousFrame();

    HRESULT hr;

    hr = m_commandAllocators[m_frameIndex]->Reset();

    if (FAILED(hr))
    {
        return;
    }

    hr = m_commandList->Reset(m_commandAllocators[m_frameIndex], nullptr);

    if (FAILED(hr)) 
    {
        return;
    }

    CD3DX12_RESOURCE_BARRIER presentToRtTransition = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    m_commandList->ResourceBarrier(1, &presentToRtTransition);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);

    m_commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    CD3DX12_RESOURCE_BARRIER rtToPresentTransition = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    m_commandList->ResourceBarrier(1, &rtToPresentTransition);

    hr = m_commandList->Close();

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

    hr = m_swapChain->Present(0, 0);

    if (FAILED(hr))
    {
        return;
    }
}

void D3D12Renderer::WaitForPreviousFrame()
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
