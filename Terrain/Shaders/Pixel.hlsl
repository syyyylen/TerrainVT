struct DSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD1;
};

float4 main(DSOutput input) : SV_TARGET
{
    float ambient = 0.2;
    float3 n = normalize(input.normal);
    float3 l = normalize(float3(0.0f, 1.0f, 0.4f));
    float diffuse = saturate(dot(n, l));
    float lighting = ambient + diffuse * (1.0 - ambient);
    
    return float4(lighting, lighting, lighting, 1.0f);
}