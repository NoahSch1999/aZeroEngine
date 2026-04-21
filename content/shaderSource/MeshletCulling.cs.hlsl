#include "MeshletCommon.hlsli"
#include "MeshDrawPassCommon.hlsli"

ConstantBuffer<BindingConstants> Bindings : register(b0);

struct InstanceConstant
{
    uint ID;
};
ConstantBuffer<InstanceConstant> Instance : register(b1);

[numthreads(THREADS_PER_X, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    const StructuredBuffer<InstanceData> instances = ResourceDescriptorHeap[Bindings.InstanceBuffer];
    const InstanceData instance = instances[Instance.ID];
    
    min16uint meshIndex, materialIndex;
    UnpackBatchID(instance.BatchID, meshIndex, materialIndex);
    
    const StructuredBuffer<Mesh> meshes = ResourceDescriptorHeap[Bindings.MeshBuffer];
    const Mesh mesh = meshes[meshIndex];
    
    if (mesh.MeshletCount > dtid.x)
    {
        const StructuredBuffer<Meshlet> meshlets = ResourceDescriptorHeap[mesh.MeshletBuffer];
        const Meshlet meshlet = meshlets[dtid.x];
        
        const float3 boundsWP = mul(instance.Transform, float4(meshlet.Bounds.Position, 1.f)).xyz;
        const BoundingSphere bounds = CreateBoundingSphere(boundsWP, meshlet.Bounds.Radius);

        const StructuredBuffer<CameraData> cameraBuffer = ResourceDescriptorHeap[Bindings.CameraBuffer];
        const CameraData camera = cameraBuffer[Bindings.CameraID];
        if (true/*camera.BoundingFrustum.Intersects(bounds)*/)
        {
            RWStructuredBuffer<MeshShaderIndirectArgs> indirectArgumentsBuffer = ResourceDescriptorHeap[Bindings.IndirectArgumentMeshletCullingBuffer];
            RWStructuredBuffer<MeshletCulling_To_MeshShader_Data> meshletInstanceBuffer = ResourceDescriptorHeap[Bindings.MeshletInstanceBuffer];
            uint meshletInstanceIndex;
            InterlockedAdd(indirectArgumentsBuffer[0].GroupsX, 1, meshletInstanceIndex);

            meshletInstanceBuffer[meshletInstanceIndex].InstanceID = Instance.ID;
            meshletInstanceBuffer[meshletInstanceIndex].LocalMeshletIndex = dtid.x;
        }
    }
}