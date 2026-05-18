#include "MeshDefinitions.hlsli"
#include "Util.hlsli"
#include "PayloadDefinitions.hlsli"
#include "GeometryPipeline_IA.hlsli"
#include "Camera.hlsli"

ConstantBuffer<IndirectArgumentConstantData> Input_CONSTANT : register(b0);

ConstantBuffer<Camera> CameraBuffer : register(b1);

StructuredBuffer<MeshInstanceData> MeshInstances : register(t0);

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
    const MeshInstanceData meshInstance = MeshInstances[Input_CONSTANT.MeshInstanceIndex.x]; // Top stall???? Doesn't seem like it
    if (meshInstance.Instance.MeshletCount > meshletIndex)
    {
        const StructuredBuffer<Meshlet> Meshlets = ResourceDescriptorHeap[meshInstance.Instance.MeshBufferIndex];
        const Meshlet meshlet = Meshlets[payload.MeshletIndex[meshletIndex]]; // Top stall
        SetMeshOutputCounts(meshlet.VertexCount, meshlet.PrimitiveCount);
    
        if (localThreadIndex < meshlet.PrimitiveCount)
        {
            const StructuredBuffer<uint> primitiveBuffer = ResourceDescriptorHeap[meshInstance.Instance.MeshBufferIndex + 1];
            tris[localThreadIndex] = Unpack32To8(primitiveBuffer[meshlet.PrimitiveOffset + localThreadIndex]);
        }
    
        if (localThreadIndex < meshlet.VertexCount)
        {
            const StructuredBuffer<uint> indices = ResourceDescriptorHeap[meshInstance.Instance.MeshBufferIndex + 3];
            uint vertexIndex = indices[meshlet.VertexOffset + localThreadIndex];
            
            const StructuredBuffer<MeshVertex> vertexPositionBuffer = ResourceDescriptorHeap[meshInstance.Instance.MeshBufferIndex + 2];
            
            RasterVertex newVertex;
            GetVertex(newVertex, CameraBuffer.ViewProjectionMatrix, vertexPositionBuffer[vertexIndex] /* Top stall */, meshInstance.Instance.WorldTransform);
            verts[localThreadIndex] = newVertex;
        }
    }
    
    //const MeshInstanceData meshInstance = MeshInstances[Input_CONSTANT.MeshInstanceIndex.x]; // Top stall???? Doesn't seem like it
    //if (meshInstance.Instance.MeshletCount > meshletIndex)
    //{
    //    const StructuredBuffer<Meshlet> Meshlets = ResourceDescriptorHeap[meshInstance.Instance.MeshBufferIndex];
    //    const Meshlet meshlet = Meshlets[payload.MeshletIndex[meshletIndex]]; // Top stall
    //    SetMeshOutputCounts(meshlet.VertexCount, meshlet.PrimitiveCount);
    
    //    uint vertexIndex;
    //    if (localThreadIndex < meshlet.VertexCount)
    //    {
    //        const StructuredBuffer<uint> indices = ResourceDescriptorHeap[meshInstance.Instance.MeshBufferIndex + 3];
    //        vertexIndex = indices[meshlet.VertexOffset + localThreadIndex];
    //    }

    //    if (localThreadIndex < meshlet.PrimitiveCount)
    //    {
    //        const StructuredBuffer<uint> primitiveBuffer = ResourceDescriptorHeap[meshInstance.Instance.MeshBufferIndex + 1];
    //        tris[localThreadIndex] = Unpack32To8(primitiveBuffer[meshlet.PrimitiveOffset + localThreadIndex]);
    //    }

    //    if (localThreadIndex < meshlet.VertexCount)
    //    {
       
        
    //        const StructuredBuffer<MeshVertex> vertexPositionBuffer = ResourceDescriptorHeap[meshInstance.Instance.MeshBufferIndex + 2];
        
    //        RasterVertex newVertex;
    //        GetVertex(newVertex, CameraBuffer.ViewProjectionMatrix, vertexPositionBuffer[vertexIndex] /* Top stall */, meshInstance.Instance.WorldTransform);
    //        verts[localThreadIndex] = newVertex;
    //    }
    //}
}