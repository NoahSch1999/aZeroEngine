float3 HashColor(uint id)
{
    // Simple hash → color
    float r = frac(sin(id * 12.9898) * 43758.5453);
    float g = frac(sin(id * 78.233) * 43758.5453);
    float b = frac(sin(id * 39.3467) * 43758.5453);
    return float3(r, g, b);
}