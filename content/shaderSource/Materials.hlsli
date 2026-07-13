struct PBRMaterial
{
    // todo Use the albedo alpha channel for roughness and normal w channel for metallic
    uint AlbedoTexture;
    uint NormalMap;
    uint RoughnessMetallic;
    uint GlowMap;
    uint TransparencyMap;
};