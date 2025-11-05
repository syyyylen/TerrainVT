struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

struct HSOutput
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD0;
};

struct HSConstantOutput
{
    float edges[3] : SV_TessFactor;
    float inside : SV_InsideTessFactor;
};

/*
Texture2D t1 : register(t0);
SamplerState s1 : register(s0);
*/

#define TESS_FACTOR 64;

HSConstantOutput HSConstant(InputPatch<VSOutput, 3> patch, uint patchID : SV_PrimitiveID)
{
    HSConstantOutput output;
    
    output.edges[0] = TESS_FACTOR;
    output.edges[1] = TESS_FACTOR;
    output.edges[2] = TESS_FACTOR;
    output.inside = TESS_FACTOR;
    
    return output;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("HSConstant")]
[maxtessfactor(4.0f)]
HSOutput main(InputPatch<VSOutput, 3> patch, uint i : SV_OutputControlPointID, uint patchID : SV_PrimitiveID)
{
    HSOutput output;
    output.pos = patch[i].pos.xyz;
    output.uv = patch[i].uv;
    
    /*
    float height = t1.SampleLevel(s1, output.uv, 0).r;
    float3 pos = patch[i].pos.xyz;
    pos.y += height * 25.0f;
    output.pos = pos;
    */
    
    return output;
}