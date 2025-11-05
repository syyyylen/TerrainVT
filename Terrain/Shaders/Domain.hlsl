struct HSConstantOutput
{
    float edges[3] : SV_TessFactor;
    float inside : SV_InsideTessFactor;
};

struct HSOutput
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD0;
};

struct DSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer ConstantBuffer : register(b0)
{
    row_major float4x4 viewProj;
};

Texture2D t1 : register(t0);
SamplerState s1 : register(s0);

[domain("tri")]
DSOutput main(HSConstantOutput input, float3 bary : SV_DomainLocation, const OutputPatch<HSOutput, 3> patch)
{
    DSOutput output;
    
    float3 pos = patch[0].pos * bary.x + patch[1].pos * bary.y + patch[2].pos * bary.z;
    
    output.uv = patch[0].uv * bary.x + patch[1].uv * bary.y + patch[2].uv * bary.z;
    
    float height = t1.SampleLevel(s1, output.uv, 0).r * 25.0f;
    pos.y += height;
    
    output.pos = mul(float4(pos, 1.0f), viewProj);
    
    return output;
}