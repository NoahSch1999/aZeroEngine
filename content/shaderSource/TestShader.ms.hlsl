struct Out
{
    float4 pos : SV_Position;
};

struct ConstantsData
{
    uint i;
};
ConstantBuffer<ConstantsData> Constants : register(b0);

[NumThreads(3, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    uint dtid : SV_DispatchThreadID,
    out vertices Out verts[3],
    out indices uint3 tris[1]
)
{
    SetMeshOutputCounts(3, 1);
    
    if (gtid < 1)
    {
        tris[gtid] = 0;
    }
    
    if (gtid < 3)
    {
        verts[gtid].pos = gtid + Constants.i;
    }
}