#include "SceneRenderCommon.hlsli"

// Root constants passed via indirect draw
/*
    struct From_CS_ConstantsData
    {
        uint MeshInstanceIndex; // Mesh instance index to look into MeshInstance_Bindless
        uint MaterialIndex; // Material instance
    };
*/
ConstantBuffer<From_CS_ConstantsData> From_CS_Constants : register(b0);

// Set from the cpu like normal
struct PassConstantData
{
    uint MeshInstance_Bindless; // Bindless index to the mesh instances buffer
    float4x4 CameraView; // Camera view matrix
    Frustum CameraFrustum; // Camera frustum
};
ConstantBuffer<PassConstantData> PassConstants : register(b1);

groupshared MeshletPayload payload; // Payload that has an array of THREADS_PER_X elements. Each element has info of a meshlet to draw in the mesh shader

    // THIS IS INCORRECT!!!
    // THIS IS INCORRECT!!!
    // THIS IS INCORRECT!!!
    // THIS IS INCORRECT!!!
    // THIS IS INCORRECT!!!
    // THIS IS INCORRECT!!!
    // THIS IS INCORRECT!!!
    // THIS IS INCORRECT!!!
    // THIS IS INCORRECT!!!
    // THIS IS INCORRECT!!!
    // THIS IS INCORRECT!!!
    // THIS IS INCORRECT!!!
    // THIS IS INCORRECT!!!
    // THIS IS INCORRECT!!!
    // THIS IS INCORRECT!!!
    // THIS IS INCORRECT!!!
[NumThreads(THREADS_PER_X, 1, 1)]
void main(uint gtid : SV_GroupThreadID)
{
    bool visible = false;

    StructuredBuffer<MeshInstance> meshInstanceBuffer = ResourceDescriptorHeap[PassConstants.MeshInstance_Bindless];
    const MeshInstance meshInstance = meshInstanceBuffer[From_CS_Constants.MeshInstanceIndex];
    
    StructuredBuffer<Meshlet> meshletBuffer = ResourceDescriptorHeap[meshInstance.MeshletBuffer_Bindless]; // Buffer containing the offset into the vertex buffer and the vertex count for a single meshlet
    
    // Check so we dont exceed the meshlet count of the mesh instance
    if (gtid < meshInstance.MeshletCount)
    {
        // Perform frustum culling
        const Meshlet meshlet = meshletBuffer[gtid];
        const float3 boundsWP = mul(meshInstance.WorldTransform, float4(meshlet.Bounds.Position, 1.f)).xyz;
        const BoundingSphere bounds = CreateBoundingSphere(boundsWP, meshlet.Bounds.Radius);
        visible = true; //PassConstants.CameraFrustum.Intersects(bounds, PassConstants.CameraView);
    }

    // Add visible meshlets into the payload that is sent to the mesh shader
    if (visible)
    {
        const Meshlet meshlet = meshletBuffer[gtid];
        const uint index = WavePrefixCountBits(visible); // Gets the number of visible meshlets and appends this meshlet's relevant data to the next empty spot in the payload array
        payload.data[index].VertexOffset = meshlet.VertexOffset;
        payload.data[index].VertexCount = meshlet.VertexCount;
    }

    // Dispatch the required number of MS threadgroups to render the visible meshlets
    // I *think* this is OK since the threads will run in lock-step, but I'm not sure...
    const uint visibleCount = WaveActiveCountBits(visible);
    DispatchMesh(visibleCount, 1, 1, payload);
}