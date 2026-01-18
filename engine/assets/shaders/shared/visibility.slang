module visibility;

import math;
import view;
import material;
import voxel;
import svt64;
import bindless;

/* Voxel visibility data. */
public struct VisData {
    public uint object_index;
    public uint voxel_index;
    /* Encodes the axis using 2 bits and the sign with 1 bit. (AAS) */
    public uint face_index;
};

/* Voxel geometry data. */
public struct GeoData {
    public float depth;
    public float3 normal;
    public Material material;
};

/* Voxel face geometry data. */
public struct VoxelFacet {
    public float3 center;
    public float3 normal;
};

/* Pack voxel visibility into two 32-bit unsigned integers. */
public [ForceInline] uint64_t pack_visibility(const Hit hit) {
    if (hit.t == BIG_F32) return uint64_t::maxValue;
    /* face_index (3) | object_index (16) | voxel_index (32) */
    return (uint64_t)hit.voxel_index | ((uint64_t)hit.object_index << 32) | ((uint64_t)hit.face_index << 48);
}

/* Unpack voxel visibility. */
public [ForceInline] VisData unpack_visibility(const uint64_t packed) {
    /* face_index (3) | object_index (16) | voxel_index (32) */
    return VisData(
        (uint)((packed >> 32) & 0xFFFFu),
        (uint)(packed & 0xFFFFFFFFu),
        (uint)(packed >> 48),
    );
}

/* Returns true if the visibility data doesn't contain a surface. */
public [ForceInline] bool null_visibility(const uint64_t packed) {
    return packed == uint64_t::maxValue;
}

/* Expands one dimensional voxel index into three dimensional voxel position. */
public [ForceInline] uint3 expand_voxel_index(const uint voxel_index) {
    return uint3(voxel_index & 0b1111111111u, (voxel_index >> 10) & 0b1111111111u, voxel_index >> 20);
}

/* Finds the material for a given voxel pointer inside a SVT64. */
public [ForceInline] Material voxel_material(const VoxelObject object, const uint voxel_ptr) {
    /* Find the dword and bit offset for the voxel pointer */
    const uint dword_ptr = voxel_ptr >> 2u; /* voxel_ptr / 4u */
    const uint bit_offset = (voxel_ptr & 0b11u) << 3u; /* (voxel_ptr % 4u) * 8 bits */

    /* Fetch the bindless voxel material buffer */
    const StructuredBuffer<uint> voxels = bindless::storage_buffer<StructuredBuffer<uint>>(object.voxels_handle);

    /* Use the dword and bit offset to get the palette index for the voxel */
    const uint palette_index = voxels[dword_ptr] >> bit_offset & 0xFFu;

    /* Fetch the bindless material palette buffer */
    const StructuredBuffer<Material> palette = bindless::storage_buffer<StructuredBuffer<Material>>(object.palette_handle);
    return palette[palette_index];
}

/* Unpack voxel geometry from visibility data. */
public [ForceInline] GeoData unpack_geometry(const VisData vis, StructuredBuffer<VoxelObject> objects, const Ray ray) {
    GeoData data;
    const VoxelObject object = objects[vis.object_index];

    /* Unpack the geometry normal (1b sign | 2b axis) */
    float3 local_normal = 0.0f;
    local_normal[vis.face_index & 0b11u] = (vis.face_index >> 2) == 0u ? -1.0f : 1.0f;
    data.normal = normalize(mul(object.local_to_world, float4(local_normal, 0.0f)).xyz);

    /* Unpack the voxel position index (1D -> 3D) */
    const uint3 voxel_index = expand_voxel_index(vis.voxel_index);

    /* Calculate the voxel face position in world-space */
    const float3 voxel_pos = (float3(voxel_index) + 0.5f + local_normal * 0.5f) * UNITS_PER_VOXEL;
    const float3 half_extent = float3(object.size_rcp_width.xyz) * 0.5f * UNITS_PER_VOXEL;
    const float3 world_voxel = mul(object.local_to_world, float4(voxel_pos - half_extent, 1.0f)).xyz;

    /* Find the depth of the geometry */
    data.depth = dot(world_voxel - ray.origin, data.normal) / dot(data.normal, ray.dir);

    /* Find the voxel data pointer by traversing the SVT64 */
    const StructuredBuffer<Svt64Node> nodes = bindless::storage_buffer<StructuredBuffer<Svt64Node>>(object.blas_handle);
    const uint voxel_ptr = fetch_svt64(nodes, object.tree_depth, voxel_index);

    /* Find the voxel material */
    data.material = voxel_material(object, voxel_ptr);
    
    return data;
}

/* Unpack voxel facet from visibility data. */
public [ForceInline] VoxelFacet unpack_geometry(const VisData vis, const StructuredBuffer<VoxelObject> objects) {
    const VoxelObject object = objects[vis.object_index];

    /* Unpack the geometry normal (1b sign | 2b axis) */
    float3 local_normal = 0.0f;
    local_normal[vis.face_index & 0b11u] = (vis.face_index >> 2) == 0u ? -1.0f : 1.0f;
    const float3 facet_normal = normalize(mul(object.local_to_world, float4(local_normal, 0.0f)).xyz);

    /* Unpack the voxel position index (1D -> 3D) */
    const uint3 voxel_index = expand_voxel_index(vis.voxel_index);

    /* Calculate the voxel face position in world-space */
    const float3 local_voxel = (float3(voxel_index) + 0.5f + local_normal * 0.5f) * UNITS_PER_VOXEL;
    const float3 half_extent = float3(object.size_rcp_width.xyz) * 0.5f * UNITS_PER_VOXEL;
    const float3 facet_center = mul(object.local_to_world, float4(local_voxel - half_extent, 1.0f)).xyz;
    return VoxelFacet(facet_center, facet_normal);
}
