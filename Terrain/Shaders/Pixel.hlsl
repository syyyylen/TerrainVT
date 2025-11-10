struct DSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD1;
};

Texture2D diffuseTexture : register(t0);
SamplerState s1 : register(s0);

float4 main(DSOutput input) : SV_TARGET
{
    float3 finalColor = diffuseTexture.Sample(s1, input.uv).rgb;

    return float4(finalColor, 1.0f);
}