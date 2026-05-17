#ifndef PAYLOAD_DEFINITIONS_INCLUDED
#define PAYLOAD_DEFINITIONS_INCLUDED

struct Payload
{
    uint MeshletIndex[32];
};

// This avoids the stall of waiting on the meshlet data in the mesh shader, but comes at the cost of a bigger payload...
// Maybe pack them as 16bit...? VertexCount and PrimitiveCount might need to be 32bit if we wanna support meshes with >4mil vertices.
// The offsets need to be 32bit if we have one big meshlet buffer for all geometry in the scene...
// So this might scale poorly...
//struct Payload
//{
//    uint VertexOffset[32];
//    uint VertexCount[32];
//    uint PrimitiveOffset[32];
//    uint PrimitiveCount[32];
//};

#endif