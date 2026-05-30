#include "MeshDefinitions.hlsli"
#include "PayloadDefinitions.hlsli"
#include "GeometryPipeline_IA.hlsli"
#include "Camera.hlsli"

groupshared Payload payload;

ConstantBuffer<IndirectArgumentConstantData> Input_CONSTANT : register(b0);

ConstantBuffer<Camera> CameraBuffer : register(b1);

StructuredBuffer<MeshInstanceData> MeshInstances : register(t0);

[NumThreads(32, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID, uint3 gid : SV_GroupThreadID)
{
    const MeshInstanceData meshInstance = MeshInstances[Input_CONSTANT.MeshInstanceIndex.x];
    bool visible = false;
    if (dtid.x < meshInstance.Instance.MeshletCount)
    {
        const StructuredBuffer<Meshlet> Meshlets = ResourceDescriptorHeap[meshInstance.Instance.MeshBufferIndex];
        const Meshlet meshlet = Meshlets[meshInstance.Instance.MeshletOffset + dtid.x];
        
        visible = CameraBuffer.Frustum.Intersects(CreateBoundingSphere(mul(meshInstance.Instance.WorldTransform, float4(meshlet.Bounds.Position, 1.f)).xyz, meshlet.Bounds.Radius), CameraBuffer.ViewMatrix);
        
        if (visible)
        {
            payload.MeshletIndex[WavePrefixCountBits(visible)] = meshInstance.Instance.MeshletOffset + dtid.x;
        }
    }
    
    DispatchMesh(WaveActiveCountBits(visible), 1, 1, payload);
}