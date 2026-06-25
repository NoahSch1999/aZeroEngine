#include "GPU_Structs.hlsli"

groupshared Payload payload;

ConstantBuffer<IndirectArgumentConstantData> Input_CONSTANT : register(b0);

ConstantBuffer<CameraData> CameraDataBuffer : register(b1);

StructuredBuffer<InstanceData> InstanceDataBufferAS : register(t0);
StructuredBuffer<BoundingSphere> MeshletBoundBuffer : register(t1);

[NumThreads(TREADS_PER_WAVE, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID, uint3 gid : SV_GroupThreadID)
{
    bool visible = false;
    if (dtid.x < Input_CONSTANT.MeshletCount)
    {
        BoundingSphere meshletBounds = MeshletBoundBuffer[Input_CONSTANT.GlobalMeshletOffset + dtid.x];
        visible = CameraDataBuffer.Frustum.Intersects(CreateBoundingSphere(mul(Input_CONSTANT.Instance, float4(meshletBounds.Position, 1.f)).xyz, meshletBounds.Radius), CameraDataBuffer.ViewMatrix);
        
        if (visible)
        {
            payload.MeshletIndex[WavePrefixCountBits(visible)] = Input_CONSTANT.GlobalMeshletOffset + dtid.x;
        }
    }
    
    DispatchMesh(WaveActiveCountBits(visible), 1, 1, payload);
}