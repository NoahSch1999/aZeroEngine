struct MeshletCulling_To_MeshShader_Data
{
    uint InstanceID;
    uint LocalMeshletIndex;
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