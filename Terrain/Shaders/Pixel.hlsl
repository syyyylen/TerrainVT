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

float4 main(DSOutput input) : SV_TARGET
{
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);
    
    const int textureSize = 16; // 16x16px tex
    const int pageSize = 4; // 4x4px pages
    
    float2 rqPx = floor(input.uv * textureSize);
    float2 rqPage = floor(rqPx / pageSize);
    float2 rqPageUV = rqPage / 3;
    
    float4 pagetableSample = vtPagetable.Sample(s1, rqPageUV);
    if (pagetableSample.a > 0.0f)
    {
        float2 physicalAdress = pagetableSample.rg * 255.0f;
        float2 pageAdress = physicalAdress / textureSize; // back to 0-1 range along texture
        
        finalColor = vtTexture.Sample(s1, pageAdress + 0.1f /* we are at the corner of the page, move inside it */).rgb;
    }
    
    
    return float4(finalColor, 1.0f);
    
    // return float4(input.uv, 0.0f, 1.0f);
    // float3 finalColor = diffuseTexture.Sample(s1, input.uv).rgb;
    // return float4(finalColor, 1.0f);
}