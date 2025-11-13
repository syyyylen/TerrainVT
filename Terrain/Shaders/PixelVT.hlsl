struct DSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD1;
};

float4 main(DSOutput input) : SV_TARGET
{
    const int textureSize = 16; // 16x16px tex
    const int pageSize = 4; // 4x4px pages
    
    float2 rqPx = floor(input.uv * textureSize);
    float2 rqPage = floor(rqPx / pageSize);
    
    return float4(rqPage / 3, 0.0f, 1.0f);
}