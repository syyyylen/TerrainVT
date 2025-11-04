struct HSConstantOutput
{
    float edges[3] : SV_TessFactor;
    float inside : SV_InsideTessFactor;
};

struct HSOutput
{
    float3 pos : POSITION;
};

struct DSOutput
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
};

cbuffer ConstantBuffer : register(b0)
{
    row_major float4x4 viewProj;
};

[domain("tri")]
DSOutput main(HSConstantOutput input, float3 bary : SV_DomainLocation, const OutputPatch<HSOutput, 3> patch)
{
    DSOutput output;
    
    float3 pos = patch[0].pos * bary.x + patch[1].pos * bary.y + patch[2].pos * bary.z;
    
    pos.y = pos.y + cos(pos.x) * 1.2f;
    
    output.pos = mul(float4(pos, 1.0f), viewProj);
    
    float3 tangent = float3(1.0f, -sin(pos.x) * 1.2f, 0.0f);
    float3 bitangent = float3(0.0f, 0.0f, 1.0f);
    
    output.normal = normalize(cross(tangent, bitangent));
    
    return output;
}