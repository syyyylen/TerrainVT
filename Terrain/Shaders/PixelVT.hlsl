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
};

float4 main(DSOutput input) : SV_TARGET
{
    float2 rqPx = floor(input.uv * vt_texture_size);
    float2 rqPage = floor(rqPx / vt_texture_page_size);
    
    return float4(rqPage / (vt_texture_page_size - 1), 0.0f, 1.0f);
}