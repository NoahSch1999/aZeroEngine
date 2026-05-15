struct Meshlet
{
    uint VertexOffset;
    uint VertexCount;
    uint PrimitiveOffset;
    uint PrimitiveCount;
    BoundingSphere Bounds;
};

struct Payload
{
    uint MeshletIndex[32];
};

groupshared Payload payload;

struct Input_IA
{
    float4x4 WorldTransform;
    uint MeshBufferIndex;
    uint MeshletOffset;
    uint MeshletCount;
    uint MaterialIndex;
};
ConstantBuffer<Input_IA> Input_CONSTANTS : register(b0);

[NumThreads(32, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID, uint3 gid : SV_GroupThreadID)
{
    bool visible = false;
    if (dtid.x < MeshletCount)
    {
        const StructuredBuffer<Meshlet> Meshlets = ResourceDescriptorHeap[Input_CONSTANTS.MeshBufferIndex];
        const Meshlet meshlet = Meshlets[MeshletOffset + dtid.x];
        
        // TODO: Add culling
        //const BoundingSphere bounds = CreateBoundingSphere(mul(MeshletDrawConstants.WorldTransform, float4(meshlet.Bounds.Position, 1.f)).xyz, meshlet.Bounds.Radius);
        visible = true;
        
        if (visible)
        {
            const uint index = WavePrefixCountBits(visible); 
            payload.MeshletIndex[index] = dtid.x;
        }
    }
    
    const uint visibleMeshletsInWave = WaveActiveCountBits(visible);
    DispatchMesh(visibleMeshletsInWave, 1, 1, payload);
}