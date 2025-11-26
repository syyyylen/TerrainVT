#include "Shaders/PerlinNoise.hlsli"

cbuffer ConstantBuffer : register(b0)
{
    float4x4 viewProj;
    float noise_persistence;
    float noise_lacunarity;
    float noise_scale;
    float noise_height;
    int noise_octaves;
    int runtime_noise;
    int vt_texture_size;
    int vt_texture_page_size;
    int vt_main_memory_texture_size;
    float vt_texture_tiling;
    float2 padding;
};

RWTexture2D<float> OutHeightMap : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height;
    OutHeightMap.GetDimensions(width, height);

    if (DTid.x >= width || DTid.y >= height)
        return;

    float u = (float)DTid.x / (float)(width - 1);
    float v = (float)DTid.y / (float)(height - 1);

    float scale = noise_scale;
    float noise = fbm(u * scale, v * scale, noise_octaves, noise_persistence, noise_lacunarity);

    OutHeightMap[DTid.xy] = noise;
}
