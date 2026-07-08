#ifndef UTIL_INCLUDED
#define UTIL_INCLUDED

float3 HashColor(uint id)
{
    // Simple hash → color
    float r = frac(sin(id * 12.9898) * 43758.5453);
    float g = frac(sin(id * 78.233) * 43758.5453);
    float b = frac(sin(id * 39.3467) * 43758.5453);
    return float3(r, g, b);
}

uint3 Unpack32To8(uint input)
{
    uint ch1, ch2, ch3;
    ch1 = (input & 0x000000ff);
    ch2 = (input & 0x0000ff00) >> 8;
    ch3 = (input & 0x00ff0000) >> 16;
    return uint3(ch1, ch2, ch3);
}

uint Pack16To32(uint2 input)
{
    return (input.x & 0xFFFF) | ((input.y & 0xFFFF) << 16);
}

uint2 Unpack32To16(uint input)
{
    uint ch1, ch2;
    ch1 = (input & 0x0000ffff);
    ch2 = (input & 0xffff0000) >> 16;
    return uint2(ch1, ch2);
}

void Matrix4x3_To_Matrix4x4(in float4x3 input, out float4x4 output)
{
    output[0] = float4(input[0], 0);
    output[1] = float4(input[1], 0);
    output[2] = float4(input[2], 0);
    output[3] = float4(input[3], 1);
}

float3 DecodeNormalOctahedral(float2 e)
{
    float3 n = float3(e.x, e.y, 1.0 - abs(e.x) - abs(e.y));

    if (n.z < 0.0)
    {
        float2 signNotZero = float2(
            n.x >= 0.0 ? 1.0 : -1.0,
            n.y >= 0.0 ? 1.0 : -1.0
        );

        n.xy = (1.0 - abs(n.yx)) * signNotZero;
    }

    return normalize(n);
}

// Thanks Frisvad
void CalcTangentAndBitangent(float3 N, out float3 T, out float3 B)
{
    float s = sign(N.z);
    float a = -1.0 / (s + N.z);
    float b = N.x * N.y * a;

    T = float3(
        1.0 + s * N.x * N.x * a,
        s * b,
        -s * N.x
    );

    B = float3(
        b,
        s + N.y * N.y * a,
        -N.y
    );
}

float2 UnpackOct16(uint2 p)
{
    float2 n;
    n.x = (p.x / 65535.0) * 2.0 - 1.0;
    n.y = (p.y / 65535.0) * 2.0 - 1.0;
    return n;
}

float2 UnpackUV16(uint2 p)
{
    return p / 65535.0;
}

float2 Unpack32ToHalfFloats(uint p)
{
    return float2(f16tof32(p & 0xFFFF), f16tof32(p >> 16));
}

#endif