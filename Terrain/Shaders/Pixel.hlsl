struct DSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

Texture2D t1 : register(t0);
SamplerState s1 : register(s0);

float4 main(DSOutput input) : SV_TARGET
{
    float3 texColor = t1.Sample(s1, input.uv).xyz;
    
    return float4(texColor, 1.0f);
}