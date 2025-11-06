struct DSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(DSOutput input) : SV_TARGET
{
    return float4(input.uv.x, input.uv.y, 0.0f, 1.0f);
}