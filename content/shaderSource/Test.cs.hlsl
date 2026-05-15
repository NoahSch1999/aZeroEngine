#include "SceneRenderCommon.hlsli"

ConstantBuffer<GPUDrivenRenderConstants> Constants_CONSTANT : register(b0);
StructuredBuffer<MeshInstance> MeshInstances : register(t0);
RWStructuredBuffer<MeshCull_Count> MeshInstanceIndexCounter : register(u0);
RWStructuredBuffer<MeshletCull_IA> MeshletCullPass_IA : register(u1);

// Split the number of mesh instance across THREADS_PER_X threads per group
[numthreads(THREADS_PER_X, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (Constants_CONSTANT.MeshInstancesCount > dtid.x)
    {

        //if (frust.Intersects(bounds, view))
        {
            uint meshObjectIndex;
            InterlockedAdd(MeshInstanceIndexCounter[0].Count, 1, meshObjectIndex);
            MeshletCullPass_IA[meshObjectIndex].MeshInstance.WorldTransform = MeshInstances[dtid.x].WorldTransform;
            MeshletCullPass_IA[meshObjectIndex].MeshInstance.MeshletCount = MeshInstances[dtid.x].MeshletCount;
            MeshletCullPass_IA[meshObjectIndex].MeshInstance.MeshBuffer_Bindless = MeshInstances[dtid.x].MeshBuffer_Bindless;
            MeshletCullPass_IA[meshObjectIndex].MaterialConstants.MaterialIndex = MeshInstances[dtid.x].MaterialIndex;
            MeshletCullPass_IA[meshObjectIndex].GroupsX = ceil(MeshInstances[dtid.x].MeshletCount / (float) THREADS_PER_X); // Splits the passed mesh instance's meshlets across THREADS_PER_X num groups (rounded up)
            MeshletCullPass_IA[meshObjectIndex].GroupsY = 1;
            MeshletCullPass_IA[meshObjectIndex].GroupsZ = 1;
        }
    }
}