struct FragmentInput
{
    float4 position : SV_Position;
};

struct FragmentOutput
{
    float4 outputTarget : SV_TARGET0;
};

StructuredBuffer<float3> ColorBuffer : register(t0);

FragmentOutput main(FragmentInput input)
{
    FragmentOutput output;
    output.outputTarget = float4(ColorBuffer.Load((int) input.position.x), 1.f);
    return output;
}