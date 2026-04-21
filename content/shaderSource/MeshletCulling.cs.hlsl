#include "MeshletCommon.hlsli"
#include "MeshDrawPassCommon.hlsli"

ConstantBuffer<BindingConstants> Bindings : register(b0);
ConstantBuffer<InstanceCulling_To_MeshletCulling_Data> PassedConstants : register(b1);

[numthreads(64, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    const StructuredBuffer<InstanceData> instances = ResourceDescriptorHeap[Bindings.InstanceBuffer];
    const InstanceData instance = instances[0/*PassedConstants.InstanceID*/];
    
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
            RWStructuredBuffer<IndirectArgument> indirectArgumentsBuffer = ResourceDescriptorHeap[Bindings.IndirectArgumentBuffer];
            RWStructuredBuffer<MeshletCulling_To_MeshShader_Data> meshletInstanceBuffer = ResourceDescriptorHeap[Bindings.MeshletInstanceBuffer];
            uint meshletInstanceIndex;
            InterlockedAdd(indirectArgumentsBuffer[0].GroupsX, 1, meshletInstanceIndex);

            meshletInstanceBuffer[meshletInstanceIndex].InstanceID = 0; //PassedConstants.InstanceID;
            meshletInstanceBuffer[meshletInstanceIndex].LocalMeshletIndex = dtid.x;
        }
    }
}