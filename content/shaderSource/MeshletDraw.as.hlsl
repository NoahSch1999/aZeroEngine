#include "SceneRenderCommon.hlsli"

ConstantBuffer<MeshletDrawConstantsData> MeshletDrawConstants : register(b0); // Passed from MeshCull compute shader pass
ConstantBuffer<GPUDrivenRenderConstants> ConstantsAS : register(b1);

groupshared MeshletPayload payload; // Payload that has an array of THREADS_PER_X elements. Each element has info of a meshlet to draw in the mesh shader

[NumThreads(THREADS_PER_X, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID, uint3 gid : SV_GroupThreadID)
{
    if (gid.x == 0)
    {
        payload.Constants.WorldTransform = MeshletDrawConstants.WorldTransform;
        payload.Constants.MaterialIndex = MeshletDrawConstants.MaterialIndex;
        payload.Constants.MeshBuffer_Bindless = MeshletDrawConstants.MeshBuffer_Bindless;
        payload.Constants.MeshletCount = MeshletDrawConstants.MeshletCount;
    }
    
    bool visible = false;
    uint vertexOffset;
    uint triangleCount;
    if (dtid.x < MeshletDrawConstants.MeshletCount)
    {
        const StructuredBuffer<Meshlet> Meshlets = ResourceDescriptorHeap[MeshletDrawConstants.MeshBuffer_Bindless];
        const Meshlet meshlet = Meshlets[dtid.x];
        vertexOffset = meshlet.VertexOffset;
        triangleCount = meshlet.TriangleCount;
        
        const float3 boundsWP = mul(MeshletDrawConstants.WorldTransform, float4(meshlet.Bounds.Position, 1.f)).xyz;
        const BoundingSphere bounds = CreateBoundingSphere(boundsWP, meshlet.Bounds.Radius);
        
        //float4x4 view;
        //view._11_12_13_14 = float4(-1, 0, 0, 0);
        //view._21_22_23_24 = float4(0, 1, 0, 0);
        //view._31_32_33_34 = float4(0, 0, -1, 0);
        //view._41_42_43_44 = float4(10, 0, -2, 1);
        
        //BoundingFrustum frust;
        //frust.Rotation = float4(0, 0, 0, 1);
        //frust.Position = float3(10, 0, -2);
        //frust.RightSlope = -1.77636278;
        //frust.LeftSlope = 1.77636278;
        //frust.TopSlope = -0.999204040;
        //frust.BottomSlope = 0.999204040;
        //frust.Near = -1092.26672;
        //frust.Far = -0.00100000005;
        
        ////isible = Constants.CameraFrustum.Intersects(bounds, Constants.CameraView);
        //visible = frust.Intersects(bounds, view);
        //if (dtid.x % 2 == 0)
            visible = true;
    }
    
    if (visible)
    {
        const uint index = WavePrefixCountBits(visible); // Gets the number of visible meshlets and appends this meshlet's relevant data to the next empty spot in the payload array
        payload.VertexOffset[index] = vertexOffset;
        payload.TriangleCount[index] = triangleCount;
    }
    
    const uint visibleCount = WaveActiveCountBits(visible);
    payload.VisibilityCount = visibleCount;
    DispatchMesh(visibleCount, 1, 1, payload);
}