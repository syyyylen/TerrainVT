RWTexture2D<float> OutputTexture : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height;
    OutputTexture.GetDimensions(width, height);

    if (DTid.x >= width || DTid.y >= height)
        return;

    OutputTexture[DTid.xy] = 0.0;
}
