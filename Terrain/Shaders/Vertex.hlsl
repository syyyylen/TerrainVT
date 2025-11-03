cbuffer ConstantBuffer : register(b0)
{
    row_major float4x4 viewProj;
};

float4 main(float3 pos : POSITION) : SV_POSITION
{
    float4 retPos = mul(float4(pos, 1.0), viewProj);
    
    return float4(retPos);
}