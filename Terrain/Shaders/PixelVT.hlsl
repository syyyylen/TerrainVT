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
    int vt_texture_size; // TODO Demain : we need both texture size & vt texture size
    int vt_texture_page_size;
};

float4 main(DSOutput input) : SV_TARGET
{
    float vtWidthRatio = 1.0f; // (vt width / final render pass width) ratio, for now both passes have the same size & resolution
    float vtHeightRatio = 1.0f; // same for height for now
    float2 dx = ddx(input.uv * vt_texture_size) * vtWidthRatio;
    float2 dy = ddy(input.uv * vt_texture_size) * vtHeightRatio;

    float d = max(dot(dx, dx), dot(dy, dy));
    float mip = 0.5f * log2(d);
    
    float maxMipLevel = log2(vt_texture_size / vt_texture_page_size);
    mip = clamp(mip, 0.f, maxMipLevel); // between 0-N mips levels
    mip = floor(mip);
    
    float2 rqPx = floor(input.uv * vt_texture_size) / exp2(mip);
    float2 rqPage = floor(rqPx / vt_texture_page_size);
    float pagetableSize = (vt_texture_size / exp2(mip)) / vt_texture_page_size;

    rqPage = min(rqPage, pagetableSize - 1.0f); // edge case when uv are at the edge

    return float4(rqPage / pagetableSize, mip / 255.0f, 1.0f);
}