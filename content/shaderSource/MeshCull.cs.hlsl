#include "SceneRenderCommon.hlsli"

ConstantBuffer<GPUDrivenRenderConstants> Constants : register(b0);
StructuredBuffer<MeshInstance> MeshInstances : register(t0);
RWStructuredBuffer<MeshCull_Count> MeshInstanceIndexCounter : register(u0);
RWStructuredBuffer<MeshletCull_IA> MeshletCullPass_IA : register(u1);

// Split the number of mesh instance across THREADS_PER_X threads per group
[numthreads(THREADS_PER_X, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (Constants.MeshInstancesCount > dtid.x)
    {
        const MeshInstance meshInstance = MeshInstances[dtid.x]; // Each thread from 0-n accesses an unique mesh instance
        
        // Perform frustum culling on the mesh instance
        const float3 boundsWP = mul(meshInstance.WorldTransform, float4(meshInstance.MeshBounds.Position, 1.f)).xyz;
        const BoundingSphere bounds = CreateBoundingSphere(boundsWP, meshInstance.MeshBounds.Radius);
        //if (Constants.CameraFrustum.Intersects(bounds, Constants.CameraView))
        {
            uint meshObjectIndex;
            InterlockedAdd(MeshInstanceIndexCounter[0].Count, 1, meshObjectIndex);
            MeshletCullPass_IA[meshObjectIndex].MeshInstance.WorldTransform = meshInstance.WorldTransform;
            MeshletCullPass_IA[meshObjectIndex].MeshInstance.MeshletCount = meshInstance.MeshletCount;
            MeshletCullPass_IA[meshObjectIndex].MeshInstance.MeshletBuffer_Bindless = meshInstance.MeshletBuffer_Bindless;
            MeshletCullPass_IA[meshObjectIndex].MeshInstance.MaterialIndex = meshInstance.MaterialIndex;
            MeshletCullPass_IA[meshObjectIndex].GroupsX = ceil(meshInstance.MeshletCount / (float) THREADS_PER_X); // Splits the passed mesh instance's meshlets across THREADS_PER_X num groups (rounded up)
            MeshletCullPass_IA[meshObjectIndex].GroupsY = 1;
            MeshletCullPass_IA[meshObjectIndex].GroupsZ = 1;
        }
    }
}