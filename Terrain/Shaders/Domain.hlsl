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
    bool noise_runtime;
    float2 padding;
};

Texture2D heightMap : register(t1);
SamplerState samplerState : register(s0);

[domain("tri")]
DSOutput main(HSConstantOutput input, float3 bary : SV_DomainLocation, const OutputPatch<HSOutput, 3> patch)
{
    DSOutput output;
    
    float3 pos = patch[0].pos * bary.x + patch[1].pos * bary.y + patch[2].pos * bary.z;
    
    output.uv = patch[0].uv * bary.x + patch[1].uv * bary.y + patch[2].uv * bary.z;
    
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
    
    output.worldPos = pos;
    output.pos = mul(float4(pos, 1.0f), viewProj);
    
    return output;
}