#include "Texture.h"
#include "Include/d3dx12.h"
#include <iostream>

Image::~Image()
{
    if (bytes != nullptr)
    {
        delete[] bytes;
        bytes = nullptr;
    }
}

void Image::LoadImageFromFile(const std::string& path, bool flip, int desiredChannels)
{
    int channels;

    stbi_set_flip_vertically_on_load(flip);
    bytes = reinterpret_cast<char*>(stbi_load(path.c_str(), &width, &height, &channels, desiredChannels));
    if (!bytes)
    {
        std::cout << "Failed to load image " + path << std::endl;
        return;
    }

    std::cout << "Loaded image " + path << std::endl;
}

void Texture::LoadFromFile(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* commandList,
    const std::string& filepath,
    DXGI_FORMAT textureFormat,
    bool flip)
{
    int desiredChannels = STBI_rgb_alpha;
    int bytesPerPixel = 4;

    if (textureFormat == DXGI_FORMAT_R8_UNORM)
    {
        desiredChannels = STBI_grey;
        bytesPerPixel = 1;
    }

    Image img;
    img.LoadImageFromFile(filepath, flip, desiredChannels);

    format = textureFormat;
    width = img.width;
    height = img.height;

    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Alignment = 0;
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.MipLevels = 1;
    textureDesc.Format = format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    CD3DX12_HEAP_PROPERTIES heapPropDefault(D3D12_HEAP_TYPE_DEFAULT);

    HRESULT hr = device->CreateCommittedResource(
        &heapPropDefault,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&resource));

    if (FAILED(hr))
        return;

    UINT64 uploadBufferSize;
    device->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

    CD3DX12_HEAP_PROPERTIES heapPropUpload(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

    hr = device->CreateCommittedResource(
        &heapPropUpload,
        D3D12_HEAP_FLAG_NONE,
        &uploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadHeap));

    if (FAILED(hr))
        return;

    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = img.bytes;
    textureData.RowPitch = width * bytesPerPixel;
    textureData.SlicePitch = width * bytesPerPixel * height;

    UpdateSubresources(commandList, resource, uploadHeap, 0, 0, 1, &textureData);

    CD3DX12_RESOURCE_BARRIER textureTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        resource,
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    commandList->ResourceBarrier(1, &textureTransition);
}

void Texture::CreateEmpty(
    ID3D12Device* device,
    int texWidth,
    int texHeight,
    DXGI_FORMAT textureFormat,
    D3D12_RESOURCE_FLAGS flags,
    D3D12_RESOURCE_STATES initialState)
{
    format = textureFormat;
    width = texWidth;
    height = texHeight;

    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Alignment = 0;
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.MipLevels = 1;
    textureDesc.Format = format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDesc.Flags = flags;

    CD3DX12_HEAP_PROPERTIES heapPropDefault(D3D12_HEAP_TYPE_DEFAULT);

    device->CreateCommittedResource(
        &heapPropDefault,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        initialState,
        nullptr,
        IID_PPV_ARGS(&resource));
}

void Texture::CreateSRV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    device->CreateShaderResourceView(resource, &srvDesc, cpuHandle);
}

void Texture::CreateUAV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = format;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;

    device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, cpuHandle);
}

void Texture::Release()
{
    if (resource)
    {
        resource->Release();
        resource = nullptr;
    }

    if (uploadHeap)
    {
        uploadHeap->Release();
        uploadHeap = nullptr;
    }
}