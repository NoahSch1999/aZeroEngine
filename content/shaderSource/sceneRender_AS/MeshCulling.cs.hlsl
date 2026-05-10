#include "SceneRenderCommon.hlsli"

// Set from the cpu like normal
struct MeshCullingPassData
{
    uint IA_MeshCullingOutput_Bindless; // Bindless index to the indirect argument buffer
    uint IA_MeshCullingCount_Bindless; // Bindless index to the indirect argument count buffer
    uint MeshInstance_Bindless; // Bindless index to the mesh instances buffer
    uint MeshInstanceCount; // Total mesh instances in MeshInstance_Bindless buffer
    float4x4 CameraView; // Camera view matrix
    Frustum CameraFrustum; // Camera frustum
};
ConstantBuffer<MeshCullingPassData> PassConstants : register(b0);

// Indirect argument that is appended per mesh instance that passes culling
struct MeshCulling_To_AmplificationShader
{
    /*
        struct From_CS_ConstantsData
        {
            uint MeshInstanceIndex; // Mesh instance index to look into MeshInstance_Bindless
            uint MaterialIndex; // Material instance
        };
    */
    From_CS_ConstantsData from_CS_Constants;
    uint GroupsX;
    uint GroupsY;
    uint GroupsZ;
};

// Split the number of mesh instance across THREADS_PER_X threads per group
[numthreads(THREADS_PER_X, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (PassConstants.MeshInstanceCount > dtid.x)
    {
        const StructuredBuffer<MeshInstance> meshInstanceBuffer = ResourceDescriptorHeap[PassConstants.MeshInstance_Bindless];
        const MeshInstance meshInstance = meshInstanceBuffer[dtid.x]; // Each thread from 0-n accesses an unique mesh instance
        
        // Perform frustum culling on the mesh instance
        const float3 boundsWP = mul(meshInstance.WorldTransform, float4(meshInstance.MeshBounds.Position, 1.f)).xyz;
        const BoundingSphere bounds = CreateBoundingSphere(boundsWP, meshInstance.MeshBounds.Radius);
        if (PassConstants.CameraFrustum.Intersects(bounds, PassConstants.CameraView))
        {
            RWStructuredBuffer<MeshCulling_To_AmplificationShader> outputArgs = ResourceDescriptorHeap[PassConstants.IA_MeshCullingOutput_Bindless]; // Indirect argument buffer
            RWStructuredBuffer<uint> passedMeshesCounterBuffer = ResourceDescriptorHeap[PassConstants.IA_MeshCullingCount_Bindless]; // Count buffer for number of passed mesh instances
            uint meshObjectIndex;
            InterlockedAdd(passedMeshesCounterBuffer[0], 1, meshObjectIndex);
            outputArgs[meshObjectIndex].from_CS_Constants.MeshInstanceIndex = dtid.x; // Forwards the mesh instance index to the rest of the pipeline through a root constant index
            outputArgs[meshObjectIndex].from_CS_Constants.MaterialIndex = meshInstance.MaterialIndex; // Forwards the mesh instance material index to the rest of the pipeline through a root constant index
            outputArgs[meshObjectIndex].GroupsX = ceil(meshInstance.MeshletCount / (float) THREADS_PER_X); // Splits the passed mesh instance's meshlets across THREADS_PER_X num groups (rounded up)
            outputArgs[meshObjectIndex].GroupsY = 1;
            outputArgs[meshObjectIndex].GroupsZ = 1;
        }
    }
}