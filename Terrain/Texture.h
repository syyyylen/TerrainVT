#pragma once
#include "Core.h"
#include "Include/stb/stb_image.h"
#include "Include/stb/stb_image_write.h"

struct Image
{
    ~Image();

    void LoadImageFromFile(const std::string& path, bool flip = true, int desiredChannels = STBI_rgb_alpha);

    char* bytes = nullptr;
    int width;
    int height;
};

struct Texture 
{
    void LoadFromFile(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* commandList,
        const std::string& filepath,
        DXGI_FORMAT textureFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
        bool flip = true);

    void CreateEmpty(
        ID3D12Device* device,
        int width,
        int height,
        DXGI_FORMAT textureFormat,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
        D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COPY_DEST);

    void CreateSRV(
        ID3D12Device* device,
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);

    void CreateUAV(
        ID3D12Device* device,
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);

    void CreateRTV(
        ID3D12Device* device,
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);

    void Release();

    void GetFootprint(ID3D12Device* device, D3D12_PLACED_SUBRESOURCE_FOOTPRINT& outFootprint, UINT64& totalBytes);

    ID3D12Resource* resource = nullptr;
    ID3D12Resource* uploadHeap = nullptr;
    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
    int width = 0;
    int height = 0;
};