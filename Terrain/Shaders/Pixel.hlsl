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
    int vt_main_memory_texture_size;
    float3 padding;
};

float4 main(DSOutput input) : SV_TARGET
{
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);

    float2 dx = ddx(input.uv * vt_texture_size);
    float2 dy = ddy(input.uv * vt_texture_size);
    float d = max(dot(dx, dx), dot(dy, dy));
    float mip = 0.5f * log2(d);

    float maxMipLevel = log2(vt_texture_size / vt_texture_page_size);
    mip = clamp(mip, 0.0f, maxMipLevel);
    mip = floor(mip); // from floating value to round mip level
    
    for (int i = mip; i <= maxMipLevel; i++) // if we don't find anything in the pagetable at mip N, we continue with mip N+1 etc...
    {
        float2 rqPx = floor(input.uv * vt_texture_size) / exp2(i);
        float2 rqPage = floor(rqPx / vt_texture_page_size);
        float pagetableSize = (vt_texture_size / exp2(i)) / vt_texture_page_size;
        float2 rqPageUV = rqPage / pagetableSize;

        float4 pagetableSample = vtPagetable.SampleLevel(s1, rqPageUV, i);
        if (pagetableSample.a > 0.0f)
        {
            // page physical address (tile index * tile size = pixel coords)
            float2 pageAdress = pagetableSample.rg * 255.0f * vt_texture_page_size;

            float2 rest = frac(rqPx / vt_texture_page_size);
            rest = rest * vt_texture_page_size;

            float2 pxAdress = pageAdress + rest; // px physical adress on main memory texture

            float2 pxUVcoords = pxAdress / vt_main_memory_texture_size; // back to 0-1 range

            float4 vTexColor = vtTexture.Sample(s1, pxUVcoords);
            if (vTexColor.a > 0.0f)
            {
                finalColor = vTexColor.rgb;
                break;
            }
        }
    }
    
    return float4(finalColor, 1.0f);

    // return float4(input.uv, 0.0f, 1.0f);
    // float3 finalColor = diffuseTexture.Sample(s1, input.uv).rgb;
    // return float4(finalColor, 1.0f);
}