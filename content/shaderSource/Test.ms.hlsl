struct SomeData
{
    uint x;
};

struct Payload
{
    uint x;
};

struct Vertex
{
    float4 Position : SV_Position;
};

ConstantBuffer<SomeData> SomeData_CONSTANT : register(b0);
ConstantBuffer<SomeData> SomeDataOther : register(b1);

[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint localThreadIndex : SV_GroupIndex, // One per vertex in the meshlet
    uint meshletIndex : SV_GroupID, // One per meshlet
    in payload Payload payload,
    out vertices Vertex verts[64],
    out indices uint3 tris[126]
)
{
    SetMeshOutputCounts(payload.x, 3);
    Vertex v;
    v.Position = float4(1, 1, 1, 1);
    tris[localThreadIndex] = uint3(payload.x, SomeData_CONSTANT.x, SomeDataOther.x);
    verts[localThreadIndex] = v;
}