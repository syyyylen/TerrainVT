#include "D3D12Renderer.h"

#include <iostream>

D3D12Renderer::D3D12Renderer()
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
}

D3D12Renderer::~D3D12Renderer()
{
}