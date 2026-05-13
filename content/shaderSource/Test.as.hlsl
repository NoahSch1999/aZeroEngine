struct SomeData
{
    uint x;
};

struct Payload
{
    uint x;
};


ConstantBuffer<SomeData> SomeData_CONSTANT : register(b0);
ConstantBuffer<SomeData> SomeDataOther : register(b1);
StructuredBuffer<uint> idk : register(t0);

groupshared Payload payload;

[NumThreads(32, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID, uint3 gid : SV_GroupThreadID)
{
    payload.x = idk[0];
    DispatchMesh(dtid.x, SomeData_CONSTANT.x, SomeDataOther.x, payload);
}