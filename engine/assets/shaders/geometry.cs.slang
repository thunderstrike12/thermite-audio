#define TRAVERSAL_STEPS

import view;
import math;
import voxel;
import bvh2;
import svt64;

import bindless;

/* Ray AABB intersection function. *(returns BIG_F32 if the ray misses the AABB)* */
inline void intersect_aabb(float3 ro, float3 rrd, float3 bmin, float3 bmax, out float hit_t) {
    const float3 t_to_min = (bmin - ro) * rrd;
    const float3 t_to_max = (bmax - ro) * rrd;
    const float3 t_min = min(t_to_min, t_to_max);
    const float3 t_max = max(t_to_min, t_to_max);
    const float t_near = max(max(max(t_min.x, t_min.y), t_min.z), 0.0f);
    const float t_far = min(min(t_max.x, t_max.y), t_max.z);
    hit_t = t_near > t_far ? BIG_F32 : t_near;
}

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Object buffers. */
StructuredBuffer<float4> bvh_nodes;
StructuredBuffer<uint> object_indices;
StructuredBuffer<VoxelObject> object_data;
/* Render target. */
RWTexture2D<float4> render_target;

inline float3 heat_color(float t) {
    t = saturate(t);
    const float3 c0 = float3(-0.002136485053939582, -0.000749655052795221, -0.005386127855323933);
    const float3 c1 = float3(0.2516605407371642, 0.6775232436837668, 2.494026599312351);
    const float3 c2 = float3(8.353717279216625, -3.577719514958484, 0.3144679030132573);
    const float3 c3 = float3(-27.66873308576866, 14.26473078096533, -13.64921318813922);
    const float3 c4 = float3(52.17613981234068, -27.94360607168351, 12.94416944238394);
    const float3 c5 = float3(-50.76852536473588, 29.04658282127291, 4.23415299384598);
    const float3 c6 = float3(18.65570506591883, -11.48977351997711, -5.601961508734096);
    return c0 + t * (c1 + t * (c2 + t * (c3 + t * (c4 + t * (c5 + t * c6)))));
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID, int group_id: SV_GroupIndex) {
    /* Generate a ray for our render view */
    const Ray ray = pinhole_ray(render_view, thread_id);

    /* Trace the scene BVH */
    const Hit hit = trace_bvh2<true>(bvh_nodes, object_indices, object_data, ray);
    // const float hit_t = trace_svt64(blas, voxels, ray, group_id);
    // render_target[thread_id] = float4(hit_t, hit_t, hit_t, 1.0f);

    if (hit.t < BIG_F32) {
        render_target[thread_id] = float4(1.0f - (hit.t * 0.01f).xxx, 1.0f);
    } else {
        const float3 color = heat_color((float)hit.steps / 128.0f);
        render_target[thread_id] = float4(color, 1.0f);
    }
}
