struct Payload
{
    uint MeshletIndex[32];
};

struct Input_IA
{
    float4x4 WorldTransform;
    uint MeshBufferIndex;
    uint MeshletOffset;
    uint MeshletCount;
    uint MaterialIndex;
};
ConstantBuffer<Input_IA> Input_CONSTANTS : register(b0);

[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint localThreadIndex : SV_GroupIndex,
    uint meshletIndex : SV_GroupID,
    in payload Payload payload,
    out vertices PipelineVertex verts[64],
    out indices uint3 tris[126]
)
{
    if (Input_CONSTANTS.MeshletCount > meshletIndex)
    {
        const StructuredBuffer<Meshlet> Meshlets = ResourceDescriptorHeap[Input_CONSTANTS.MeshBufferIndex];
        const Meshlet meshlet = Meshlets[MeshletOffset + meshletIndex];
        SetMeshOutputCounts(meshlet.VertexCount, meshlet.PrimitiveCount);
    
        if (localThreadIndex < meshlet.PrimitiveCount)
        {
            const StructuredBuffer<uint> primitiveBuffer = ResourceDescriptorHeap[Input_CONSTANTS.MeshBufferIndex + 1];
            tris[localThreadIndex] = Unpack32To8(primitiveBuffer[meshlet.PrimitiveOffset + localThreadIndex]);
        }
    
        if (localThreadIndex < meshlet.VertexCount)
        {
            
            const StructuredBuffer<uint> indices = ResourceDescriptorHeap[Input_CONSTANTS.MeshBuffer_Bindless + 3];
            uint vertexIndex = indices[meshlet.VertexOffset + localThreadIndex];
            
            const StructuredBuffer<MeshVertex> vertexPositionBuffer = ResourceDescriptorHeap[Input_CONSTANTS.MeshBufferIndex + 2];
            
            PipelineVertex newVertex;
            GetVertex(newVertex, vertexIndex, ConstantsMS.CameraVP, vertexPositionBuffer, Input_CONSTANTS.WorldTransform);
            verts[localThreadIndex] = newVertex;
        }
    }
}