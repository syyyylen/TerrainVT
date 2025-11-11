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

#define TESS_FACTOR 1;

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
    
    return output;
}