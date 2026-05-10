#include "../MeshDefinitions.hlsli"

struct MeshletPayloadElement
{
    uint VertexOffset;
    uint VertexCount;
};

struct MeshletPayload
{
    MeshletPayloadElement data[THREADS_PER_X];
};

struct From_CS_ConstantsData // From CS
{
    uint MeshInstanceIndex;
    uint MaterialIndex;
};

struct From_CS_MeshletCulling_ConstantsData // From CS
{
    uint MeshInstanceIndex;
    uint MaterialIndex;
    MeshletPayloadElement Payload;
};