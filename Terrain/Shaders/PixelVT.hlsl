struct DSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD1;
};

float4 main(DSOutput input) : SV_TARGET
{
    return float4(0.0f, 1.0f, 0.0f, 1.0f);
}