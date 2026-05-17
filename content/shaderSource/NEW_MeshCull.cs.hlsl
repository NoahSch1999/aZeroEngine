#include "NEW_MeshDefinitions.hlsli"

struct MeshCullConstantsData
{
	uint MeshInstanceCount;
};
ConstantBuffer<MeshCullConstantsData> MeshCull_CONSTANT : register(b0);

struct MeshInstanceData
{
    MeshInstance Instance;
    BoundingSphere MeshBounds;
};
StructuredBuffer<MeshInstanceData> MeshInstances : register(t0);

struct IndirectArgumentCounter
{
	uint Count;
};
RWStructuredBuffer<IndirectArgumentCounter> IndirectArgumentCounter : register(u0);

struct IndirectArguments
{
    MeshInstance Instance;
	uint GroupX;
	uint GroupY;
	uint GroupZ;
};
RWStructuredBuffer<IndirectArguments> IndirectArguments : register(u1);

[numthreads(32, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (MeshCull_CONSTANT.MeshInstanceCount > dtid.x)
    {
        const MeshInstanceData meshInstance = MeshInstances[dtid.x];

		// TODO: Add culling
		uint meshInstanceIndex;
        InterlockedAdd(IndirectArgumentCounter[0].Count, 1, meshInstanceIndex);
        IndirectArguments[meshInstanceIndex].Instance = meshInstance.Instance;
        IndirectArguments[meshInstanceIndex].GroupX = ceil(meshInstance.Instance.MeshletCount / 32.f);
        IndirectArguments[meshInstanceIndex].GroupY = 1;
        IndirectArguments[meshInstanceIndex].GroupZ = 1;
    }
}