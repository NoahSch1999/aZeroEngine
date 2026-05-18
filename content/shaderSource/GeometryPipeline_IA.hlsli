#ifndef GEO_PASS_IA_INCLUDED
#define GEO_PASS_IA_INCLUDED

struct IndirectArgumentConstantData
{
    uint MeshInstanceIndex;
    uint Pad1;
    uint Pad2;
    uint Pad3;
};

struct IndirectArgumentStruct
{
    IndirectArgumentConstantData Data;
    uint GroupX;
    uint GroupY;
    uint GroupZ;
};

#endif