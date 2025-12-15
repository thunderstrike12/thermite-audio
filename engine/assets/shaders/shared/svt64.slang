module svt64;

import view;
import math;
import material;

public struct Svt64Hit {
    public float3 point;
    public uint voxel_index;
    public uint steps;

    /* Returns true if this was a miss. */
    public bool miss() { return voxel_index == 0xFFFFFFFFu; }
};

public struct Svt64Node {
    uint child_ptr;
    uint child_mask_hi;
    uint child_mask_lo;

    /* Is this node a leaf node? */
    [ForceInline] bool is_leaf() { return (child_ptr & 0x80000000u) != 0; };

    /* Absolute offset into an array of child nodes / voxels. */
    [ForceInline] uint abs_ptr() { return child_ptr & 0x7FFFFFFFu; };

    /* Is a given child node index active? */
    [ForceInline] bool child_active(uint child_index) {
        if (child_index < 32u) {
            return (child_mask_hi >> child_index & 1u) != 0u;
        } else {
            return (child_mask_lo >> (child_index - 32u) & 1u) != 0u;
        }
    }

    /* Can we perform a coalesced skip? */
    [ForceInline] bool child_coalesced_skip(uint child_index) {
        child_index = child_index & 0b101010;
        if (child_index < 32u) {
            return (child_mask_hi >> child_index & 0x00330033u) == 0u;
        } else {
            return (child_mask_lo >> (child_index - 32u) & 0x00330033u) == 0u;
        }
    }
};

/* Ray AABB intersection function. */
[ForceInline] float intersect_aabb(Ray ray, float3 box_min, float3 box_max) {
    const float3 t_to_min = (box_min - ray.origin) * ray.rcp_dir;
    const float3 t_to_max = (box_max - ray.origin) * ray.rcp_dir;
    const float3 t_min = min(t_to_min, t_to_max);
    const float3 t_max = max(t_to_min, t_to_max);
    const float t_near = max(max(max(t_min.x, t_min.y), t_min.z), 0.0f);
    const float t_far = min(min(t_max.x, t_max.y), t_max.z);
    return t_near > t_far ? BIG_F32 : t_near;
}

/* Reverses `pos` from range [1.0, 2.0) to (2.0, 1.0] if `dir > 0`. */
[ForceInline] float3 mirror_pos(float3 pos, float3 dir, const bool range_check) {
    float3 mirrored = asfloat(asuint(pos) ^ 0x7FFFFFu);
    if (range_check && any(pos < 1.0f || pos >= 2.0f)) mirrored = 3.0f - pos;
    return select(dir > 0.0f, mirrored, pos);
}

[ForceInline] int get_node_cell_index(float3 pos, int scale_exp) {
    const uint3 cell_pos = asuint(pos) >> scale_exp & 3u;
    return cell_pos.x + cell_pos.z * 4u + cell_pos.y * 16u;
}

/* `floor(pos / scale) * scale` */
[ForceInline] float3 floor_scale(float3 pos, int scale_exp) {
    const uint mask = ~0u << scale_exp;
    return asfloat(asuint(pos) & mask);
}

/* Count number of set bits in variable range [0..width] */
[ForceInline] uint popcnt_var64(uint mask_lo, uint mask_hi, uint width) {
    uint count = 0u, himask = mask_hi;
    if (width >= 32u) {
        count = countbits(himask);
        himask = mask_lo;
    }
    const uint m = 1u << (width & 31u);
    count += countbits(himask & (m - 1u));
    return count;
}

/* Trace a ray through a SVT64. */
public inline Svt64Hit trace_svt64<let DEBUG : bool>(
    StructuredBuffer<Svt64Node> nodes,
    Ray ray
) {
    const uint group_id = spirv_asm {
        result:$$uint = OpLoad builtin(LocalInvocationIndex:uint);
    };

    /* 16x8 thread group, 11 stack slots each. */
    static groupshared uint gs_stack[16 * 8][11];

    /* Mirror ray origin for more efficient ray-octant traversal */
    ray.origin = mirror_pos(ray.origin, ray.dir, true);
    ray.rcp_dir = 1.0f / -abs(ray.dir);

    /* Mirror coordinates to simplify cell intersections */
    uint mirror_mask = 0x00u;
    if (ray.dir.x > 0.0f) mirror_mask |= 3u << 0u;
    if (ray.dir.y > 0.0f) mirror_mask |= 3u << 4u;
    if (ray.dir.z > 0.0f) mirror_mask |= 3u << 2u;

    /* Traversal state */
    uint node_index = 0u; /* root node */
    int scale_exp = 21; /* 23 mantissa bits - 2 */
    Svt64Node node = nodes[node_index];
    float3 pos = clamp(ray.origin, 1.0f, 1.9999999f);
    float3 side_dist = 0.0f;
    uint steps = 0u;
    for (;;) {
        uint child_index = get_node_cell_index(pos, scale_exp) ^ mirror_mask;

        /* Descend down the tree until we find an empty node or leaf node */
        while (node.child_active(child_index) && !node.is_leaf()) {
            /* Push the current node on the stack (at `scale_exp / 2`) */
            gs_stack[group_id][scale_exp >> 1] = node_index;

            /* Fetch the child node */
            node_index = node.abs_ptr() + popcnt_var64(node.child_mask_lo, node.child_mask_hi, child_index);
            node = nodes[node_index];

            /* Decrease the scale & get the next child index */
            scale_exp -= 2;
            child_index = get_node_cell_index(pos, scale_exp) ^ mirror_mask;
        }

        /* If this node is a leaf, check if we hit a voxel */
        if (node.child_active(child_index) && node.is_leaf()) break;

        /* Check if we can actually take a larger step based on the child mask */
        const int sub_scale_exp = node.child_coalesced_skip(child_index) ? (scale_exp + 1) : scale_exp;

        /* Compute the next position by intersecting the max cell sides */
        const float3 cell_min = floor_scale(pos, sub_scale_exp);
        side_dist = (cell_min - ray.origin) * ray.rcp_dir;
        const float tmax = min(min(side_dist.x, side_dist.y), side_dist.z);
        const int3 neighbor_max = asint(cell_min) + select(side_dist == tmax, -1, (1 << sub_scale_exp) - 1);
        pos = min(ray.origin - abs(ray.dir) * tmax, asfloat(neighbor_max));

        /* Find the first common ancestor node based on left-most carry bit */
        const uint3 diff_pos = asuint(pos) ^ asuint(cell_min);
        const int diff_exp = firstbithigh((diff_pos.x | diff_pos.y | diff_pos.z) & 0xFFAAAAAAu);

        /* Traverse back up the tree if we need to */
        if (diff_exp > scale_exp) {
            /* Break if we're exiting the root node */
            scale_exp = diff_exp;
            if (diff_exp > 21) break;

            /* Read the first common ancestor node from the stack */
            node_index = gs_stack[group_id][scale_exp >> 1];
            node = nodes[node_index];
        }

        if (DEBUG) steps++;
    }

    Svt64Hit hit;
    hit.voxel_index = 0xFFFFFFFFu;
    if (DEBUG) hit.steps = steps;

    /* If we hit a leaf node, return the voxel we hit */
    if (node.is_leaf() && scale_exp <= 21) {
        /* Find the hit position & calculate the hit distance */
        pos = mirror_pos(pos, ray.dir, false);
        hit.point = pos;

        /* Fetch the material of the voxel we hit */
        const uint child_index = get_node_cell_index(pos, scale_exp);
        const uint voxel_index = node.abs_ptr() + popcnt_var64(node.child_mask_lo, node.child_mask_hi, child_index);
        hit.voxel_index = voxel_index;
    }

    return hit;
}
