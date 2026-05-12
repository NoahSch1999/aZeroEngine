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
        const StructuredBuffer<Meshlet> Meshlets = ResourceDescriptorHeap[meshInstance.MeshBuffer_Bindless];
        const Meshlet meshlet = Meshlets[dtid.x];
        
        const float3 boundsWP = mul(meshInstance.WorldTransform, float4(meshlet.Bounds.Position, 1.f)).xyz;
        const BoundingSphere bounds = CreateBoundingSphere(boundsWP, meshlet.Bounds.Radius);
        
        float4x4 view;
        view._11_12_13_14 = float4(-1, 0, 0, 0);
        view._21_22_23_24 = float4(0, 1, 0, 0);
        view._31_32_33_34 = float4(0, 0, -1, 0);
        view._41_42_43_44 = float4(10, 0, 30, 1);

        BoundingFrustum frust;
        frust.Rotation = float4(0, 0, 0, 1);
        frust.Position = float3(10, 0, 30);
        frust.RightSlope = -1.77636278;
        frust.LeftSlope = 1.77636278;
        frust.TopSlope = -0.999204040;
        frust.BottomSlope = 0.999204040;
        frust.Near = -1092.26672;
        frust.Far = -0.00100000005;

//isible = Constants.CameraFrustum.Intersects(bounds, Constants.CameraView);
        if (frust.Intersects(bounds, view))
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