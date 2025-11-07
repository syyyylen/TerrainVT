struct DSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD1;
};

Texture2D diffuseTexture : register(t0);
Texture2D computeOutputTexture : register(t1);
SamplerState s1 : register(s0);

float4 main(DSOutput input) : SV_TARGET
{
    float ambient = 0.2;
    float3 n = normalize(input.normal);
    float3 l = normalize(float3(0.0f, 1.0f, 0.4f));
    float diffuse = saturate(dot(n, l));
    float lighting = ambient + diffuse * (1.0 - ambient);

    // float heightValue = computeOutputTexture.Sample(s1, input.uv * 20.0f).r;
    // float3 finalColor = float3(heightValue, heightValue, heightValue) * lighting;
    
    float3 finalColor = diffuseTexture.Sample(s1, input.uv).rgb * lighting;

    return float4(finalColor, 1.0f);
}