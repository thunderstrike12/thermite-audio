module bvh2;

import view;
import voxel;

public struct Bvh2Node {
    /* .xyz = Left minimum bounds ~ .w = Left child node index */
    float4 lmin_left;
    /* .xyz = Left maximum bounds ~ .w = Right child node index */
    float4 lmax_right;
    /* .xyz = Right minimum bounds ~ .w = Primitive index */
    float4 rmin_prim_index;
    /* .xyz = Right maximum bounds ~ .w = Primitive count */
    float4 rmax_prim_count;

    property float3 lmin { get { return lmin_left.xyz; } }
    property float3 lmax { get { return lmax_right.xyz; } }
    property float3 rmin { get { return rmin_prim_index.xyz; } }
    property float3 rmax { get { return rmax_prim_count.xyz; } }

    property uint left { get { return asuint(lmin_left.w); } }
    property uint right { get { return asuint(lmax_right.w); } }
    property uint prim_index { get { return asuint(rmin_prim_index.w); } }
    property uint prim_count { get { return asuint(rmax_prim_count.w); } }

    /* Returns true if this node is a leaf node. */
    [ForceInline] bool is_leaf() { return prim_count > 0u; }
};

/* Trace a ray through a BVH2. */
public [ForceInline] Hit trace_bvh2<let DEBUG : bool>(
    StructuredBuffer<float4> nodes,
    StructuredBuffer<uint> indices,
    StructuredBuffer<VoxelObject> prims,
    Ray ray, float tmax = 1e30f
) {
    /* Check to make sure the root node of the BVH is valid */
    if (asuint(nodes[0].w) == 0u && asuint(nodes[3].w) == 0u) {
        return Hit::miss();
    }

    /* Prepare for slab tests */
    const float3 rcp_origin = ray.origin * -ray.rcp_dir;

    /* Traversal state */
    Hit hit = Hit::miss();
    hit.t = tmax;
    uint node_index = 0u, stack[14], stack_ptr = 0u;
    for (;;) {
        const float4 rmax_prim_count = nodes[node_index + 3u];
        const float4 rmin_prim_index = nodes[node_index + 2u];
        const uint prim_count = asuint(rmax_prim_count.w);

        /* Leaf node */
        if (prim_count > 0u) {
            /* Intersect all primitives */
            const uint prim_index = asuint(rmin_prim_index.w);
            for (uint i = 0u; i < prim_count; ++i) {
                const uint index = indices[prim_index + i];
                const VoxelObject prim = prims[index];
                trace_voxel_object<DEBUG>(ray, prim, index, hit);
            }

            /* Pop the node stack */
            if (stack_ptr == 0u) break;
            node_index = stack[--stack_ptr];
            continue;
        }

        const float4 lmin_left = nodes[node_index];
        const float4 lmax_right = nodes[node_index + 1u];

        /* Interior node */
        uint left = asuint(lmin_left.w) * 4u; 
        uint right = asuint(lmax_right.w) * 4u;
        const float3 t1a = fma(lmin_left.xyz, ray.rcp_dir, rcp_origin);
        const float3 t2a = fma(lmax_right.xyz, ray.rcp_dir, rcp_origin);
        const float3 t1b = fma(rmin_prim_index.xyz, ray.rcp_dir, rcp_origin);
        const float3 t2b = fma(rmax_prim_count.xyz, ray.rcp_dir, rcp_origin);
        const float3 minta = min(t1a, t2a);
        const float3 maxta = max(t1a, t2a);
        const float3 mintb = min(t1b, t2b);
        const float3 maxtb = max(t1b, t2b);
        const float tmina = max(max(max(minta.x, minta.y), minta.z), 0.0f);
        const float tminb = max(max(max(mintb.x, mintb.y), mintb.z), 0.0f);
        const float tmaxa = min(min(min(maxta.x, maxta.y), maxta.z), hit.t);
        const float tmaxb = min(min(min(maxtb.x, maxtb.y), maxtb.z), hit.t);
        float dist1 = tmina > tmaxa ? 1e30f : tmina;
        float dist2 = tminb > tmaxb ? 1e30f : tminb;
        if (DEBUG) hit.steps++;

        /* Swap traversal order to traverse nearest node first */
        if (dist1 > dist2) {
            float t = dist1; dist1 = dist2; dist2 = t;
            uint i = left; left = right; right = i;
        }

        /* If we missed both child nodes */
        if (dist1 == 1e30f) {
            /* Pop the node stack */
            if (stack_ptr == 0u) break;
            node_index = stack[--stack_ptr];
        } else {
            /* Continue with the closest child node */
            node_index = left;
            /* Push the 2nd child onto the node stack if we hit it */
            if (dist2 != 1e30f) stack[stack_ptr++] = right;
        }
    }

    return hit;
}

/* Trace a ray through a BVH2 to check for occlusion. */
public [ForceInline] float occlusion_bvh2(
    StructuredBuffer<float4> nodes,
    StructuredBuffer<uint> indices,
    StructuredBuffer<VoxelObject> prims,
    Ray ray, float tmax
) {
    /* Check to make sure the root node of the BVH is valid */
    if ((asuint(nodes[0].w) == 0u && asuint(nodes[3].w) == 0u) || tmax <= 0.0f) {
        return tmax;
    }

    /* Prepare for slab tests */
    const float3 rcp_origin = ray.origin * -ray.rcp_dir;

    /* Traversal state */
    float t = tmax;
    uint node_index = 0u, stack[14], stack_ptr = 0u;
    for (;;) {
        const float4 rmax_prim_count = nodes[node_index + 3u];
        const float4 rmin_prim_index = nodes[node_index + 2u];
        const uint prim_count = asuint(rmax_prim_count.w);

        /* Leaf node */
        if (prim_count > 0u) {
            /* Intersect all primitives */
            const uint prim_index = asuint(rmin_prim_index.w);
            for (uint i = 0u; i < prim_count; ++i) {
                const uint index = indices[prim_index + i];
                const VoxelObject prim = prims[index];
                occlusion_voxel_object(ray, prim, index, t);
            }

            /* Pop the node stack */
            if (stack_ptr == 0u) break;
            node_index = stack[--stack_ptr];
            continue;
        }

        const float4 lmin_left = nodes[node_index];
        const float4 lmax_right = nodes[node_index + 1u];

        /* Interior node */
        uint left = asuint(lmin_left.w) * 4u;
        uint right = asuint(lmax_right.w) * 4u;
        const float3 t1a = fma(lmin_left.xyz, ray.rcp_dir, rcp_origin);
        const float3 t2a = fma(lmax_right.xyz, ray.rcp_dir, rcp_origin);
        const float3 t1b = fma(rmin_prim_index.xyz, ray.rcp_dir, rcp_origin);
        const float3 t2b = fma(rmax_prim_count.xyz, ray.rcp_dir, rcp_origin);
        const float3 minta = min(t1a, t2a);
        const float3 maxta = max(t1a, t2a);
        const float3 mintb = min(t1b, t2b);
        const float3 maxtb = max(t1b, t2b);
        const float tmina = max(max(max(minta.x, minta.y), minta.z), 0.0f);
        const float tminb = max(max(max(mintb.x, mintb.y), mintb.z), 0.0f);
        const float tmaxa = min(min(min(maxta.x, maxta.y), maxta.z), t);
        const float tmaxb = min(min(min(maxtb.x, maxtb.y), maxtb.z), t);
        float dist1 = tmina > tmaxa ? 1e30f : tmina;
        float dist2 = tminb > tmaxb ? 1e30f : tminb;

        /* Swap traversal order to traverse nearest node first */
        if (dist1 > dist2) {
            float t = dist1; dist1 = dist2; dist2 = t;
            uint i = left; left = right; right = i;
        }

        /* If we missed both child nodes */
        if (dist1 == 1e30f) {
            /* Pop the node stack */
            if (stack_ptr == 0u) break;
            node_index = stack[--stack_ptr];
        } else {
            /* Continue with the closest child node */
            node_index = left;
            /* Push the 2nd child onto the node stack if we hit it */
            if (dist2 != 1e30f) stack[stack_ptr++] = right;
        }
    }

    return t;
}
