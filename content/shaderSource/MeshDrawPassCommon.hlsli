struct MeshletCulling_To_MeshShader_Data
{
    float4x4 Transform;
    uint BatchID;
    
    uint VertCount;
    uint VertOffset;
    uint PrimCount;
    uint PrimOffset;
    
    uint PrimitiveBuffer;
    uint IndicesBuffer;
    uint PositionBuffer;
    uint VertexDataBuffer;
};

struct MeshShaderIndirectArgs
{
    uint GroupsX;
    uint GroupsY;
    uint GroupsZ;
};

struct MeshletInstanceIndirectArgs
{
    uint InstanceID;
    uint GroupsX;
    uint GroupsY;
    uint GroupsZ;
};

#define THREADS_PER_X 64