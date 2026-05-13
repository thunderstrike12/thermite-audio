import view;
import voxel;
import bvh2;
import visibility;
import math;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
Texture2D<float4> blue_noise1d;
/* Object buffers. */
StructuredBuffer<float4> bvh_nodes;
StructuredBuffer<uint> object_indices;
StructuredBuffer<VoxelObject> object_data;
/* Visibility buffer. */
RWTexture2D<uint2> vbuffer;
/* Motion Vector Buffer */
RWTexture2D<float2> mbuffer;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (any(thread_id >= render_view.resolution))
        return;

    /* Generate a ray for our render view */
    const Ray ray = pinhole_ray(render_view, thread_id);

    /* Trace the scene BVH */
    const float dither = r1(blue_noise1d[thread_id % 512].r, render_view.frame_index % 1024u);
    const Hit hit = trace_bvh2<false>(bvh_nodes, object_indices, object_data, ray, BIG_F32, dither);

    /* Write packed visibility info into the visibility buffer */
    vbuffer[thread_id] = asuint2(pack_visibility(hit));

    /* Motion Vectors */
    if (hit.t < BIG_F32) {
        const float4 hit_pos = float4(ray.origin + ray.dir * hit.t, 1.0f);

        const VoxelObject object = object_data[hit.object_index];

        const float4 local_pos = float4(mul(object.world_to_local, hit_pos).xyz, 1.0f);

        const float4 prev_pos = float4(mul(object.prev_local_to_world, local_pos).xyz, 1.0f);

        const float4 current_pos_clip = mul(render_view.world_to_clip, hit_pos);
        const float w_eps = 1e-6f;
        float2 current_ndc = current_pos_clip.xy / max(abs(current_pos_clip.w), w_eps) * sign(current_pos_clip.w);

        const float4 prev_pos_clip = mul(render_view.prev_world_to_clip, prev_pos);
        float2 prev_ndc = prev_pos_clip.xy / max(abs(prev_pos_clip.w), w_eps) * sign(prev_pos_clip.w);

        /* Subtract Jitter */
        current_ndc -= render_view.jitter;
        prev_ndc -= render_view.prev_jitter;

        float2 motion_vector = (prev_ndc - current_ndc) * float2(0.5f, 0.5f);

        if (length(motion_vector) < 1e-5f)
            motion_vector = float2(0.0f, 0.0f);

        mbuffer[thread_id] = motion_vector;
    } else {
        const float4 hit_pos = float4(ray.origin + ray.dir * hit.t, 1.0f);

        float4 curr_clip = mul(render_view.world_to_clip, hit_pos);
        const float w_eps = 1e-6f;
        float2 curr_ndc = curr_clip.xy / max(abs(curr_clip.w), w_eps) * sign(curr_clip.w);

        float4 prev_clip = mul(render_view.prev_world_to_clip, hit_pos);
        float2 prev_ndc = prev_clip.xy / max(abs(prev_clip.w), w_eps) * sign(prev_clip.w);

        curr_ndc -= render_view.jitter;
        prev_ndc -= render_view.prev_jitter;

        float2 motion_vector = (prev_ndc - curr_ndc) * float2(0.5f, 0.5f);
        mbuffer[thread_id] = motion_vector;
    }
}
