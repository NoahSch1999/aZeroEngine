struct Out
{
    float4 pos : SV_Position;
};

struct Output
{
    float4 color : SV_TARGET0;
};

Output main(Out pin)
{
    Output output;
    output.color = float4(1, 0, 0, 1.f);
    return output;
}