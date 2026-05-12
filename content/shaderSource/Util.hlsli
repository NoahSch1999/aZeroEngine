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

void Matrix4x3_To_Matrix4x4(in float4x3 input, out float4x4 output)
{
    output[0] = float4(input[0], 0);
    output[1] = float4(input[1], 0);
    output[2] = float4(input[2], 0);
    output[3] = float4(input[3], 1);
}