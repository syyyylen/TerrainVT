#if USE_PERLIN_NOISE

static const int p[512] =
{
    151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142,
    8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117,
    35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71,
    134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41,
    55, 46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89,
    18, 169, 200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226,
    250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182,
    189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43,
    172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97,
    228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239,
    107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
    138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180,
    
    151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142,
    8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117,
    35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71,
    134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41,
    55, 46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89,
    18, 169, 200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226,
    250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182,
    189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43,
    172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97,
    228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239,
    107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
    138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
};

float fade(float t)
{
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

float grad(int hash, float x, float y)
{
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : 0.0);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float perlinNoise2D(float x, float y)
{
    int xi = (int) floor(x) & 255;
    int yi = (int) floor(y) & 255;
    
    float xf = x - floor(x);
    float yf = y - floor(y);
    
    float u = fade(xf);
    float v = fade(yf);
    
    int aa = p[p[xi] + yi];
    int ab = p[p[xi] + yi + 1];
    int ba = p[p[xi + 1] + yi];
    int bb = p[p[xi + 1] + yi + 1];
    
    float g1 = grad(aa, xf, yf);
    float g2 = grad(ba, xf - 1.0, yf);
    float g3 = grad(ab, xf, yf - 1.0);
    float g4 = grad(bb, xf - 1.0, yf - 1.0);
    
    float x1 = lerp(g1, g2, u);
    float x2 = lerp(g3, g4, u);
    float result = lerp(x1, x2, v);
    
    return result;
}

float fbm(float x, float y, int octaves, float persistence, float lacunarity)
{
    float total = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    float maxValue = 0.0;
    
    for (int i = 0; i < octaves; i++)
    {
        total += perlinNoise2D(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    
    return total / maxValue;
}

#endif

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
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD1;
};

cbuffer ConstantBuffer : register(b0)
{
    row_major float4x4 viewProj;
#if USE_PERLIN_NOISE
    float noise_persistence;
    float noise_lacunarity;
    float noise_scale;
    float noise_height;
    int noise_octaves;
    float3 padding;
#endif
};

[domain("tri")]
DSOutput main(HSConstantOutput input, float3 bary : SV_DomainLocation, const OutputPatch<HSOutput, 3> patch)
{
    DSOutput output;
    
    float3 pos = patch[0].pos * bary.x + patch[1].pos * bary.y + patch[2].pos * bary.z;
    
    output.uv = patch[0].uv * bary.x + patch[1].uv * bary.y + patch[2].uv * bary.z;
    
#if USE_PERLIN_NOISE
    
    float scale = noise_scale;
    float noise = fbm(output.uv.x * scale, output.uv.y * scale, noise_octaves, noise_persistence, noise_lacunarity);
    
    pos.y += noise * noise_height;
    
    float delta = 0.01;
    
    float heightL = fbm((output.uv.x - delta) * scale, output.uv.y * scale, noise_octaves, noise_persistence, noise_lacunarity) * noise_height;
    float heightR = fbm((output.uv.x + delta) * scale, output.uv.y * scale, noise_octaves, noise_persistence, noise_lacunarity) * noise_height;
    float heightD = fbm(output.uv.x * scale, (output.uv.y - delta) * scale, noise_octaves, noise_persistence, noise_lacunarity) * noise_height;
    float heightU = fbm(output.uv.x * scale, (output.uv.y + delta) * scale, noise_octaves, noise_persistence, noise_lacunarity) * noise_height;
    
    float3 tangentX = float3(2.0 * delta, heightR - heightL, 0.0);
    float3 tangentZ = float3(0.0, heightU - heightD, 2.0 * delta);
    
    output.normal = normalize(cross(tangentZ, tangentX));
    
#else
    
    output.normal = float3(0.0, 1.0, 0.0);
    
#endif
    
    output.worldPos = pos;
    output.pos = mul(float4(pos, 1.0f), viewProj);
    
    return output;
}