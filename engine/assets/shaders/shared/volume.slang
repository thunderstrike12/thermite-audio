module volume;

import math;

namespace iv {

/* Size of the froxel grid in froxels. */
public static const int3 FROXEL_GRID_SIZE = int3(160, 90, 64);
public static const float FROXEL_NEAR_Z = 0.1f;
public static const float FROXEL_FAR_Z = 320.0f;

/* Get the position of a froxel id in world-space. */
public [ForceInline] float3 froxel_to_world(const float3 froxel, const float4x4 clip_to_world, const float near_z, const float far_z) {
    /* Calculate the normalized device z coordinate (exponential distribution) */
    const float z_center = near_z * pow(far_z / near_z, froxel.z / FROXEL_GRID_SIZE.z);
    const float z_ndc = (far_z * near_z / z_center - far_z) / (near_z - far_z);

    /* Calculate the normalize device x and y coordinates */
    const float x_ndc = froxel.x / (FROXEL_GRID_SIZE.x * 0.5f) - 1.0f;
    const float y_ndc = froxel.y / (FROXEL_GRID_SIZE.y * 0.5f) - 1.0f;

    /* Transform from clip-space to world-space */
    const float4 view = mul(clip_to_world, float4(x_ndc, y_ndc, z_ndc, 1.0f));
    return view.xyz / view.w;
}

/* Get the UVW of the froxel volume at a given world-space position. */
public [ForceInline] float3 froxel_uvw(const float3 pos, const float4x4 world_to_clip, const float near_z, const float far_z) {
    /* Transform world-space to clip-space, then NDC */
    const float4 clip = mul(world_to_clip, float4(pos, 1.0f));
    const float3 ndc = clip.xyz / clip.w;

    /* Invert the NDC x,y mapping */
    const float u = ndc.x * 0.5f + 0.5f;
    const float v = ndc.y * 0.5f + 0.5f;

    /* Recover linear depth from NDC z, then invert the exponential distribution */
    const float z_linear = far_z * near_z / (ndc.z * (near_z - far_z) + far_z);
    const float w = log(z_linear / near_z) / log(far_z / near_z);
    return float3(u, v, w);
}

}
