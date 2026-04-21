struct InstanceCulling_To_MeshletCulling_Data
{
    uint InstanceID;
};

struct MeshletCulling_To_MeshShader_Data
{
    uint InstanceID;
    uint LocalMeshletIndex;
};

struct IndirectArgument
{
    uint GroupsX;
    uint GroupsY;
    uint GroupsZ;
};