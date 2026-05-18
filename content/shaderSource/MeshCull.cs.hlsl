#include "MeshDefinitions.hlsli"
#include "GeometryPipeline_IA.hlsli"
#include "Camera.hlsli"

struct MeshCullConstantsData
{
	uint4 MeshInstanceCount;
};
ConstantBuffer<MeshCullConstantsData> MeshCull_CONSTANT : register(b0);

ConstantBuffer<Camera> CameraBuffer : register(b1);

StructuredBuffer<MeshInstanceData> MeshInstances : register(t0);

struct IndirectArgumentCounterData
{
	uint Count;
};
RWStructuredBuffer<IndirectArgumentCounterData> IndirectArgumentCounter : register(u0);
RWStructuredBuffer<IndirectArgumentStruct> IndirectArguments : register(u1);

[numthreads(32, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (MeshCull_CONSTANT.MeshInstanceCount.x > dtid.x)
    {
        const MeshInstanceData meshInstance = MeshInstances[dtid.x];

        if (CameraBuffer.Frustum.Intersects(meshInstance.MeshBounds, CameraBuffer.ViewMatrix))
        {
            uint meshInstanceIndex;
            InterlockedAdd(IndirectArgumentCounter[0].Count, 1, meshInstanceIndex);
            IndirectArguments[meshInstanceIndex].Data.MeshInstanceIndex.x = dtid.x;
            IndirectArguments[meshInstanceIndex].GroupX = ceil(meshInstance.Instance.MeshletCount / 32.f);
            IndirectArguments[meshInstanceIndex].GroupY = 1;
            IndirectArguments[meshInstanceIndex].GroupZ = 1;
        }
    }
}