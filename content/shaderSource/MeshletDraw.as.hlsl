#include "SceneRenderCommon.hlsli"

ConstantBuffer<MeshletDrawConstantsData> MeshletDrawConstants : register(b0); // Passed from MeshCull compute shader pass
//ConstantBuffer<GPUDrivenRenderConstants> ConstantsAS : register(b1);

groupshared MeshletPayload payload; // Payload that has an array of THREADS_PER_X elements. Each element has info of a meshlet to draw in the mesh shader

[NumThreads(THREADS_PER_X, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID, uint3 gid : SV_GroupThreadID)
{
    if (gid.x == 0)
    {
        payload.WorldTransform = MeshletDrawConstants.WorldTransform;
        payload.MeshBuffer_Bindless = MeshletDrawConstants.MeshBuffer_Bindless;
    }
    
    bool visible = false;
    if (dtid.x < MeshletDrawConstants.MeshletCount)
    {
        const StructuredBuffer<Meshlet> Meshlets = ResourceDescriptorHeap[MeshletDrawConstants.MeshBuffer_Bindless];
        const Meshlet meshlet = Meshlets[dtid.x];
        
        const BoundingSphere bounds = CreateBoundingSphere(mul(MeshletDrawConstants.WorldTransform, float4(meshlet.Bounds.Position, 1.f)).xyz, meshlet.Bounds.Radius);
        visible = true;
        
        if (visible)
        {
            const uint index = WavePrefixCountBits(visible); // Gets the number of visible meshlets and appends this meshlet's relevant data to the next empty spot in the payload array
            payload.VertexOffset[index] = meshlet.VertexOffset;
            payload.VertexCount[index] = meshlet.VertexCount;
            payload.PrimitiveOffset[index] = meshlet.PrimitiveOffset;
            payload.PrimitiveCount[index] = meshlet.PrimitiveCount;
        }
    }
    
    const uint visibleMeshletsInWave = WaveActiveCountBits(visible);
    payload.VisibleMeshletsCount = visibleMeshletsInWave;
    DispatchMesh(visibleMeshletsInWave, 1, 1, payload);
}