#include "SceneRenderCommon.hlsli"

struct IA_ConstantsData
{
    uint MeshInstanceIndex;
};

ConstantBuffer<IA_ConstantsData> IA_Constants : register(b0);
ConstantBuffer<GPUDrivenRenderConstants> Constants : register(b1);

StructuredBuffer<MeshInstance> MeshInstances : register(t0);
RWStructuredBuffer<MeshletDraw_IA> MeshletDrawPass_IA : register(u0);
RWStructuredBuffer<MeshletDrawInstance> MeshletDrawInstances : register(u1);

[numthreads(THREADS_PER_X, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    const MeshInstance meshInstance = MeshInstances[IA_Constants.MeshInstanceIndex];
    
    if (meshInstance.MeshletCount > dtid.x)
    {
        const StructuredBuffer<Meshlet> Meshlets = ResourceDescriptorHeap[meshInstance.MeshletBuffer_Bindless];
        const Meshlet meshlet = Meshlets[dtid.x];
        
        const float3 boundsWP = mul(meshInstance.WorldTransform, float4(meshlet.Bounds.Position, 1.f)).xyz;
        const BoundingSphere bounds = CreateBoundingSphere(boundsWP, meshlet.Bounds.Radius);
        
        //if (Constants.CameraFrustum.Intersects(bounds, Constants.CameraView))
        {
            uint meshletInstanceIndex;
            InterlockedAdd(MeshletDrawPass_IA[0].GroupsX, 1, meshletInstanceIndex);

            MeshletDrawInstances[meshletInstanceIndex].MeshInstanceIndex = IA_Constants.MeshInstanceIndex;
            MeshletDrawInstances[meshletInstanceIndex].VertexOffset = meshlet.VertexOffset;
            MeshletDrawInstances[meshletInstanceIndex].TriangleCount = meshlet.TriangleCount;
        }
    }
}