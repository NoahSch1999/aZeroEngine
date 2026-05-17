#include "NEW_MeshDefinitions.hlsli"
#include "Util.hlsli"
#include "PayloadDefinitions.hlsli"

struct Input_IA
{
    MeshInstance Instance;
};
ConstantBuffer<Input_IA> Input_CONSTANT : register(b0);

struct CameraConstantData
{
    float4x4 ViewProjMatrix;
};
ConstantBuffer<CameraConstantData> Camera_CONSTANT : register(b1);

[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint localThreadIndex : SV_GroupIndex,
    uint meshletIndex : SV_GroupID,
    in payload Payload payload,
    out vertices RasterVertex verts[64],
    out indices uint3 tris[126]
)
{
    if (Input_CONSTANT.Instance.MeshletCount > meshletIndex)
    {
        const StructuredBuffer<Meshlet> Meshlets = ResourceDescriptorHeap[Input_CONSTANT.Instance.MeshBufferIndex];
        const Meshlet meshlet = Meshlets[payload.MeshletIndex[meshletIndex]]; // Top stall
        SetMeshOutputCounts(meshlet.VertexCount, meshlet.PrimitiveCount);
    
        if (localThreadIndex < meshlet.PrimitiveCount)
        {
            const StructuredBuffer<uint> primitiveBuffer = ResourceDescriptorHeap[Input_CONSTANT.Instance.MeshBufferIndex + 1];
            tris[localThreadIndex] = Unpack32To8(primitiveBuffer[meshlet.PrimitiveOffset + localThreadIndex]);
        }
    
        if (localThreadIndex < meshlet.VertexCount)
        {
            
            const StructuredBuffer<uint> indices = ResourceDescriptorHeap[Input_CONSTANT.Instance.MeshBufferIndex + 3];
            uint vertexIndex = indices[meshlet.VertexOffset + localThreadIndex];
            
            const StructuredBuffer<MeshVertex> vertexPositionBuffer = ResourceDescriptorHeap[Input_CONSTANT.Instance.MeshBufferIndex + 2];
            
            RasterVertex newVertex;
            GetVertex(newVertex, Camera_CONSTANT.ViewProjMatrix, vertexPositionBuffer[vertexIndex] /* Top stall */, Input_CONSTANT.Instance.WorldTransform);
            verts[localThreadIndex] = newVertex;
        }
    }
    
    //if (Input_CONSTANT.Instance.MeshletCount > meshletIndex)
    //{
    //    SetMeshOutputCounts(payload.VertexCount[meshletIndex], payload.PrimitiveCount[meshletIndex]);

    //    if (localThreadIndex < payload.PrimitiveCount[meshletIndex])
    //    {
    //        const StructuredBuffer<uint> primitiveBuffer = ResourceDescriptorHeap[Input_CONSTANT.Instance.MeshBufferIndex + 1];
    //        tris[localThreadIndex] = Unpack32To8(primitiveBuffer[payload.PrimitiveOffset[meshletIndex] + localThreadIndex]);
    //    }

    //    if (localThreadIndex < payload.VertexCount[meshletIndex])
    //    {
        
    //        const StructuredBuffer<uint> indices = ResourceDescriptorHeap[Input_CONSTANT.Instance.MeshBufferIndex + 3];
    //        uint vertexIndex = indices[payload.VertexOffset[meshletIndex] + localThreadIndex];
        
    //        const StructuredBuffer<MeshVertex> vertexPositionBuffer = ResourceDescriptorHeap[Input_CONSTANT.Instance.MeshBufferIndex + 2];
        
    //        RasterVertex newVertex;
    //        GetVertex(newVertex, Camera_CONSTANT.ViewProjMatrix, vertexPositionBuffer[vertexIndex] /* Top stall */, Input_CONSTANT.Instance.WorldTransform);
    //        verts[localThreadIndex] = newVertex;
    //    }
    //}
}