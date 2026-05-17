#include "NEW_MeshDefinitions.hlsli"
#include "PayloadDefinitions.hlsli"

groupshared Payload payload;

struct Input_IA
{
    MeshInstance Instance;
};
ConstantBuffer<Input_IA> Input_CONSTANT : register(b0);

struct CameraCullConstantData
{
    BoundingFrustum Frustum;
};
ConstantBuffer<CameraCullConstantData> CameraCull_CONSTANT : register(b1);

[NumThreads(32, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID, uint3 gid : SV_GroupThreadID)
{
    bool visible = false;
    if (dtid.x < Input_CONSTANT.Instance.MeshletCount)
    {
        const StructuredBuffer<Meshlet> Meshlets = ResourceDescriptorHeap[Input_CONSTANT.Instance.MeshBufferIndex];
        const Meshlet meshlet = Meshlets[Input_CONSTANT.Instance.MeshletOffset + dtid.x];
        
        // TODO: Add culling
        //const BoundingSphere bounds = CreateBoundingSphere(mul(MeshletDrawConstants.WorldTransform, float4(meshlet.Bounds.Position, 1.f)).xyz, meshlet.Bounds.Radius);
        visible = true;
        
        if (visible)
        {
            const uint index = WavePrefixCountBits(visible); 
            payload.MeshletIndex[index] = Input_CONSTANT.Instance.MeshletOffset + dtid.x;
            //payload.VertexOffset[index] = meshlet.VertexOffset;
            //payload.VertexCount[index] = meshlet.VertexCount;
            //payload.PrimitiveOffset[index] = meshlet.PrimitiveOffset;
            //payload.PrimitiveCount[index] = meshlet.PrimitiveCount;
        }
    }
    
    const uint visibleMeshletsInWave = WaveActiveCountBits(visible);
    DispatchMesh(visibleMeshletsInWave, 1, 1, payload);
}