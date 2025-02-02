struct Input
{
    float4 Position : SV_Position;
    float3 Color : COLOR;
};

float4 main(Input input) : SV_TARGET
{
	return float4(input.Color, 1.0f);
}