#include "SceneRenderCommon.hlsli"

ConstantBuffer<MeshletDrawConstantsData> MeshletDrawConstants : register(b0); // Passed from MeshCull compute shader pass
ConstantBuffer<GPUDrivenRenderConstants> Constants : register(b1);

groupshared MeshletPayload payload; // Payload that has an array of THREADS_PER_X elements. Each element has info of a meshlet to draw in the mesh shader

[NumThreads(THREADS_PER_X, 1, 1)]
void main(uint gtid : SV_GroupThreadID, uint groupID : SV_GroupID)
{
    bool visible = false;
    uint vertexOffset;
    uint triangleCount;
    if (MeshletDrawConstants.MeshletCount > gtid)
    {
        const StructuredBuffer<Meshlet> Meshlets = ResourceDescriptorHeap[MeshletDrawConstants.MeshletBuffer_Bindless];
        const Meshlet meshlet = Meshlets[groupID * THREADS_PER_X + gtid];
        vertexOffset = meshlet.VertexOffset;
        triangleCount = meshlet.TriangleCount;
        
        const float3 boundsWP = mul(MeshletDrawConstants.WorldTransform, float4(meshlet.Bounds.Position, 1.f)).xyz;
        const BoundingSphere bounds = CreateBoundingSphere(boundsWP, meshlet.Bounds.Radius);
        //visible = Constants.CameraFrustum.Intersects(bounds, Constants.CameraView);
        visible = true;
    }
    
    if (visible)
    {
        const uint index = WavePrefixCountBits(visible); // Gets the number of visible meshlets and appends this meshlet's relevant data to the next empty spot in the payload array
        payload.VertexOffset[index] = vertexOffset;
        payload.TriangleCount[index] = triangleCount;
    }
    
    const uint visibleCount = WaveActiveCountBits(visible);
    DispatchMesh(visibleCount, 1, 1, payload);
}