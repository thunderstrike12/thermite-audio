module material;

/* Voxel material data, used in a palette. */
public struct Material {
    public uint albedo; /* Packed 10bit RGB color. */
    public uint ior_emission; /* Packed f16 ior, f16 emission. */
    public uint roughness_metallic_transmission; /* Packed 8bit roughness, metallic, and transmission. (1 byte unused) */
};
