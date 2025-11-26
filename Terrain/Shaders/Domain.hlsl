#include "Shaders/PerlinNoise.hlsli"

struct HSConstantOutput
{
    float edges[3] : SV_TessFactor;
    float inside : SV_InsideTessFactor;
};

struct HSOutput
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD0;
};

struct DSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD1;
};

cbuffer ConstantBuffer : register(b0)
{
    row_major float4x4 viewProj;
    float noise_persistence;
    float noise_lacunarity;
    float noise_scale;
    float noise_height;
    int noise_octaves;
    int noise_runtime;
    int vt_texture_size;
    int vt_texture_page_size;
    int vt_main_memory_texture_size;
    float vt_texture_tiling;
    float2 padding;
};

Texture2D heightMap : register(t1);
SamplerState samplerState : register(s0);

#define HEIGHT_MODIF 1
#define NORMALS 0

[domain("tri")]
DSOutput main(HSConstantOutput input, float3 bary : SV_DomainLocation, const OutputPatch<HSOutput, 3> patch)
{
    DSOutput output;
    
    float3 pos = patch[0].pos * bary.x + patch[1].pos * bary.y + patch[2].pos * bary.z;
    
    output.uv = patch[0].uv * bary.x + patch[1].uv * bary.y + patch[2].uv * bary.z;
    
#if HEIGHT_MODIF
    
    // TODO ugly branch here
    // TODO use define and hot reolad shader
    if (noise_runtime)
    {
        float scale = noise_scale;
        float noise = fbm(output.uv.x * scale, output.uv.y * scale, noise_octaves, noise_persistence, noise_lacunarity);
    
        pos.y += noise * noise_height;
    }
    else
    {
        float sampledHeight = heightMap.SampleLevel(samplerState, output.uv, 0).r;
        pos.y += sampledHeight * noise_height;
    }
    
#endif
    
    output.worldPos = pos;
    output.pos = mul(float4(pos, 1.0f), viewProj);
    
#if NORMALS

    float delta = 0.01f;
    float heightL, heightR, heightD, heightU;

    if (noise_runtime)
    {
        float scale = noise_scale;
        heightL = fbm((output.uv.x - delta) * scale, output.uv.y * scale, noise_octaves, noise_persistence, noise_lacunarity) * noise_height;
        heightR = fbm((output.uv.x + delta) * scale, output.uv.y * scale, noise_octaves, noise_persistence, noise_lacunarity) * noise_height;
        heightD = fbm(output.uv.x * scale, (output.uv.y - delta) * scale, noise_octaves, noise_persistence, noise_lacunarity) * noise_height;
        heightU = fbm(output.uv.x * scale, (output.uv.y + delta) * scale, noise_octaves, noise_persistence, noise_lacunarity) * noise_height;
    }
    else
    {
        heightL = heightMap.SampleLevel(samplerState, float2(output.uv.x - delta, output.uv.y), 0).r * noise_height;
        heightR = heightMap.SampleLevel(samplerState, float2(output.uv.x + delta, output.uv.y), 0).r * noise_height;
        heightD = heightMap.SampleLevel(samplerState, float2(output.uv.x, output.uv.y - delta), 0).r * noise_height;
        heightU = heightMap.SampleLevel(samplerState, float2(output.uv.x, output.uv.y + delta), 0).r * noise_height;
    }

    float3 tangentX = float3(delta * 2.0f, heightR - heightL, 0.0f);
    float3 tangentZ = float3(0.0f, heightU - heightD, delta * 2.0f);

    output.normal = normalize(cross(tangentZ, tangentX));
    
#else
    
    output.normal = float3(0.0f, 1.0f, 0.0f);
    
#endif

    return output;
}