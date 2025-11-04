struct DSOutput
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
};

float4 main(DSOutput input) : SV_TARGET
{
    float3 lightDir = normalize(float3(0.5f, 1.0f, 0.5f));
    
    float3 color = input.normal * 0.5f + 0.5f;
    
    return float4(color, 1.0f);
}