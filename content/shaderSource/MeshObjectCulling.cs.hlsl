#include "MeshletCommon.hlsli"
#include "MeshDrawPassCommon.hlsli"

struct MeshObjectCullingConstants
{
    uint NumMeshObjects;
    uint IndirectArgumentMeshCullingBuffer;
    uint PassedMeshesCounterBuffer;
};

ConstantBuffer<BindingConstants> Bindings : register(b0);

ConstantBuffer<MeshObjectCullingConstants> PassConstants : register(b1);

[numthreads(THREADS_PER_X, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (PassConstants.NumMeshObjects > dtid.x)
    {
        const StructuredBuffer<InstanceData> instances = ResourceDescriptorHeap[Bindings.InstanceBuffer];
        const InstanceData instance = instances[dtid.x];
        
        min16uint meshIndex, materialIndex;
        UnpackBatchID(instance.BatchID, meshIndex, materialIndex);
        
        const StructuredBuffer<Mesh> meshes = ResourceDescriptorHeap[Bindings.MeshBuffer];
        const Mesh mesh = meshes[meshIndex];
        
        const StructuredBuffer<CameraData> cameraBuffer = ResourceDescriptorHeap[Bindings.CameraBuffer];
        //const CameraData camera = cameraBuffer[Bindings.CameraID];
        const CameraData camera = cameraBuffer[1];
        
        const float3 boundsWP = mul(instance.Transform, float4(mesh.Bounds.xyz, 1.f)).xyz;
        const BoundingSphere bounds = CreateBoundingSphere(boundsWP, mesh.Bounds.w);
        
        if (camera.BoundingFrustum.Intersects(bounds, camera.View))
        {
            RWStructuredBuffer<MeshletInstanceIndirectArgs> instancesPassedBuffer = ResourceDescriptorHeap[PassConstants.IndirectArgumentMeshCullingBuffer];
            RWStructuredBuffer<uint> passedMeshesCounterBuffer = ResourceDescriptorHeap[PassConstants.PassedMeshesCounterBuffer];
            uint meshObjectIndex;
            InterlockedAdd(passedMeshesCounterBuffer[0], 1, meshObjectIndex);
            instancesPassedBuffer[meshObjectIndex].InstanceID = dtid.x;
            instancesPassedBuffer[meshObjectIndex].GroupsX = ceil(mesh.MeshletCount / (float)THREADS_PER_X); // TODO: Check if OK to divide by 64
            instancesPassedBuffer[meshObjectIndex].GroupsY = 1;
            instancesPassedBuffer[meshObjectIndex].GroupsZ = 1;
        }
    }
}