#include "MeshletCommon.hlsli"

groupshared Payload s_Payload;

ConstantBuffer<BindingConstants> Bindings : register(b0);

/*
For each dispatch group:
    Perform meshlet culling on N meshlets
    if visible:
        Dispatch one mesh shader per meshlet
*/
[NumThreads(32, 1, 1)]
void main(uint gtid : SV_GroupThreadID, uint dtid : SV_DispatchThreadID, uint gid : SV_GroupID)
{
    const StructuredBuffer<InstanceData> instances = ResourceDescriptorHeap[Bindings.InstanceBuffer];
    const InstanceData instance = instances[Bindings.InstanceID];
    
    min16uint meshIndex, materialIndex;
    UnpackBatchID(instance.BatchID, meshIndex, materialIndex);
    
    const StructuredBuffer<Mesh> meshes = ResourceDescriptorHeap[Bindings.MeshBuffer];
    const Mesh mesh = meshes[meshIndex];
    const StructuredBuffer<Meshlet> meshlets = ResourceDescriptorHeap[mesh.MeshletBuffer];
    

    StructuredBuffer<CameraData> CameraBuffer = ResourceDescriptorHeap[Bindings.CameraBuffer];
    const CameraData camera = CameraBuffer[Bindings.CameraID];
    
    const Frustum frustum = camera.BoundingFrustum;
    
    // TODO: Set meshletIndex to the correct index and maybe process 32 meshlets per group...
    uint meshletIndex = 0;
    const Meshlet meshlet = meshlets[meshletIndex];
    if (frustum.Intersects(meshlet.Bounds))
    {
        s_Payload.InstanceID = Bindings.InstanceID;
        s_Payload.MeshletIndex = meshletIndex;
        DispatchMesh(1, 1, 1, s_Payload);
    }
}