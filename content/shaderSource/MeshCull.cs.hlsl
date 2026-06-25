#include "GPU_Structs.hlsli"

ConstantBuffer<MeshCullConstantsData> MeshCull_CONSTANT : register(b0);

ConstantBuffer<CameraData> CameraDataBuffer : register(b1);

StructuredBuffer<ObjectCullData> ObjectCullDataBuffer : register(t0);

RWStructuredBuffer<IndirectArgumentCounter> IndirectArgumentCounterBuffer : register(u0);
RWStructuredBuffer<IndirectArguments> IndirectArgumentsBuffer : register(u1);

StructuredBuffer<InstanceData> InstanceDataBufferMS : register(t3);
[numthreads(TREADS_PER_WAVE, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x < MeshCull_CONSTANT.MeshInstanceCount)
    {
        const ObjectCullData objectCullData = ObjectCullDataBuffer[dtid.x];

        if (CameraDataBuffer.Frustum.Intersects(objectCullData.Bounds, CameraDataBuffer.ViewMatrix))
        {
            uint meshInstanceIndex;
            InterlockedAdd(IndirectArgumentCounterBuffer[0].Count, 1, meshInstanceIndex);
            IndirectArgumentsBuffer[meshInstanceIndex].Data.Instance = InstanceDataBufferMS[dtid.x].Transform;
            IndirectArgumentsBuffer[meshInstanceIndex].Data.GlobalMeshletOffset = objectCullData.GlobalMeshletOffset;
            IndirectArgumentsBuffer[meshInstanceIndex].Data.GlobalVertexOffset = objectCullData.GlobalVertexOffset;
            IndirectArgumentsBuffer[meshInstanceIndex].Data.MaterialIndex = objectCullData.MaterialIndex;
            IndirectArgumentsBuffer[meshInstanceIndex].Data.MeshletCount = objectCullData.MeshletCount;
            IndirectArgumentsBuffer[meshInstanceIndex].GroupX = ceil(objectCullData.MeshletCount / (float)TREADS_PER_WAVE);
            IndirectArgumentsBuffer[meshInstanceIndex].GroupY = 1;
            IndirectArgumentsBuffer[meshInstanceIndex].GroupZ = 1;
        }
    }
}