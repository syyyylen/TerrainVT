struct DSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD1;
};

Texture2D diffuseTexture : register(t0);
Texture2D vtTexture : register(t2);
Texture2D vtPagetable : register(t3);
SamplerState s1 : register(s0);

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
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);
    
    float2 rqPx = floor(input.uv * vt_texture_size);
    float2 rqPage = floor(rqPx / vt_texture_page_size);
    float2 rqPageUV = rqPage / (vt_texture_page_size - 1);
    
    float4 pagetableSample = vtPagetable.Sample(s1, rqPageUV);
    if (pagetableSample.a > 0.0f)
    {
        float2 pageAdress = pagetableSample.rg * 255.0f * vt_texture_page_size; // page physical address (tile index * tile size = pixel coords)
 
        float2 rest = frac(rqPx / vt_texture_page_size);
        rest = rest * vt_texture_page_size;
        
        float2 pxAdress = pageAdress + rest; // px physical adress
 
        float2 pxUVcoords = pxAdress / vt_texture_size; // back to 0-1 range
        
        float4 vTexColor = vtTexture.Sample(s1, pxUVcoords);
 
        if (vTexColor.a > 0.0f)
        {
            finalColor = vTexColor.rgb;
        }
    }
    
    return float4(finalColor, 1.0f);
    
    // return float4(input.uv, 0.0f, 1.0f);
    // float3 finalColor = diffuseTexture.Sample(s1, input.uv).rgb;
    // return float4(finalColor, 1.0f);
}