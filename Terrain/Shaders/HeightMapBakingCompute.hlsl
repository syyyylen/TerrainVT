cbuffer ConstantBuffer : register(b0)
{
    float4x4 viewProj;
    float noise_persistence;
    float noise_lacunarity;
    float noise_scale;
    float noise_height;
    int noise_octaves;
    float3 padding;
};

RWTexture2D<float> OutputTexture : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height;
    OutputTexture.GetDimensions(width, height);

    if (DTid.x >= width || DTid.y >= height)
        return;

    OutputTexture[DTid.xy] = noise_height / 150.0;
}
