struct MeshCullConstantsData
{
	uint MeshInstanceCount;
};
ConstantBuffer<MeshCullConstantsData> MeshCull_CONSTANTS : register(b0);

struct MeshInstance
{
    float4x4 WorldTransform;
    uint MeshBufferIndex;
    uint MeshletOffset;
    uint MeshletCount;
    uint MaterialIndex;
    BoundingSphere MeshBounds;
};
StructuredBuffer<MeshInstance> MeshInstances : register(t0);

struct IndirectArgumentCounter
{
	uint Count;
};
RWStructuredBuffer<IndirectArgumentCounter> IndirectArgumentCounter_IA : register(u0);

struct IndirectAruments
{
	float4x4 WorldTransform;
    uint MeshBufferIndex;
    uint MeshletOffset;
    uint MeshletCount;
    uint MaterialIndex;

	uint GroupX;
	uint GroupY;
	uint GroupZ;
};
RWStructuredBuffer<IndirectAruments> IndirectAruments_IA : register(u1);

[numthreads(THREADS_PER_X, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
	if (Constants.MeshInstancesCount > dtid.x)
    {
		const MeshInstance meshInstance = MeshInstances[dtid.x];

		// TODO: Add culling

		uint meshInstanceIndex;
        InterlockedAdd(IndirectArgumentCounter_IA[0].Count, 1, meshInstanceIndex);
		IndirectAruments_IA[meshInstanceIndex].WorldTransform = meshInstance.WorldTransform;
		IndirectAruments_IA[meshInstanceIndex].MeshBufferIndex = meshInstance.MeshBufferIndex;
		IndirectAruments_IA[meshInstanceIndex].MeshletOffset = meshInstance.MeshletOffset;
		IndirectAruments_IA[meshInstanceIndex].MeshletCount = meshInstance.MeshletCount;
		IndirectAruments_IA[meshInstanceIndex].MaterialIndex = meshInstance.MaterialIndex;
		IndirectAruments_IA[meshInstanceIndex].GroupsX = ceil(meshInstance.MeshletCount / 32.f);
        IndirectAruments_IA[meshInstanceIndex].GroupsY = 1;
        IndirectAruments_IA[meshInstanceIndex].GroupsZ = 1;
	}
}