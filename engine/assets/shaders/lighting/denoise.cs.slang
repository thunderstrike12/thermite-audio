import view;
import visibility;
import math;
import voxel;
import colorspace;
import cache;

/* Active render view */
ConstantBuffer<RenderView> render_view;
Texture2D<float4> blue_noise;
/* Voxel objects buffer */
StructuredBuffer<VoxelObject> objects;
/* Macrofacet cache buffer. */
RWStructuredBuffer<cache::Slot> macrofacet_cache;
/* Visibility buffer */
Texture2D<uint2> vbuffer;
/* Luminance buffer (input) */
Texture2D<float3> lbuffer_in;
/* Luminance buffer (output) */
RWTexture2D<float3> lbuffer_out;

struct DenoiseOptions {
    float sample_radius;
    float color_weight;
    float normal_weight;
    float pos_weight;
};

[[vk::push_constant]] DenoiseOptions options;

struct Sample {
    float3 luminance;
    float3 normal;
    float3 position;
};

Sample fetch_sample(const int2 pixel) {
    /* Load visibility buffer data */
    const uint64_t facet_key = asuint64(vbuffer[pixel << 1]);

    /* Exit early if the visibility for this pixel is null */
    if (null_visibility(facet_key)) return Sample();

    const VisData vis = unpack_visibility(facet_key);
    const VoxelObject object = objects[vis.object_index];

    /* Unpack the geometry normal (1b sign | 2b axis) */
    float3 local_normal = 0.0f;
    local_normal[vis.face_index & 0b11u] = (vis.face_index >> 2) == 0u ? -1.0f : 1.0f;
    const float3 normal = normalize(mul(object.local_to_world, float4(local_normal, 0.0f)).xyz);

    /* Unpack the voxel position index (1D -> 3D) */
    const uint3 voxel_index = expand_voxel_index(vis.voxel_index);

    /* Calculate the voxel face position in world-space */
    const float3 voxel_pos = (float3(voxel_index) + 0.5f + local_normal * 0.5f) * UNITS_PER_VOXEL;
    const float3 half_extent = float3(object.size_rcp_width.xyz) * 0.5f * UNITS_PER_VOXEL;
    const float3 world_voxel = mul(object.local_to_world, float4(voxel_pos - half_extent, 1.0f)).xyz;

    /* Find the depth of the geometry */
    // const Ray ray = pinhole_ray(render_view, vpixel);
    // const float depth = dot(world_voxel - ray.origin, normal) / dot(normal, ray.dir);

    return Sample(
        lbuffer_in[pixel],
        normal,
        world_voxel // ray.origin + ray.dir * depth
    );
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    // int2 offset = 0;
    // int2 shift = 0;
    // if (render_view.shading_rate_di == 1u) {
    //     /* Half-rate */
    //     /*  [ XY ]  */
    //     /*  [ YX ]  */
    //     offset.x = (thread_id.y + render_view.frame_index) & 0b1;
    //     shift.x = 1;
    // } else if (render_view.shading_rate_di == 2u) {
    //     /* Quarter-rate */
    //     /*  [ XY ]  */
    //     /*  [ ZW ]  */
    //     const int2 offset = int2(render_view.frame_index & 0b1, (render_view.frame_index >> 1) & 0b1);
    //     shift = 1;
    // }
    // const int2 pixel_id = (thread_id + offset) >> shift;
#if 0
    const int2 pixel_id = thread_id;

    /* Bounds check */
    if (any(pixel_id >= render_view.resolution)) return;

    const Sample center = fetch_sample(pixel_id);

    float2 noise = r2(blue_noise[pixel_id % 512].rg, render_view.frame_index % 1024u);

    float3 sum = 0.0f;
    float cum_weight = 0.0f;

    /* Take 12 blue noise distributed neighbourhood samples */
    [ForceUnroll] for (uint i = 0u; i < 12u; ++i) {
        /* Sample neighbour */
        const int2 offset = int2((r2(noise, i) * 2.0f - 1.0f) * options.sample_radius);
        const int2 pixel = pixel_id + offset;
        const Sample sample = fetch_sample(pixel);

        /* Compute edge-stopping weights */
        const float color_d = dot(center.luminance - sample.luminance, center.luminance - sample.luminance);
        const float color_w = exp(-color_d / options.color_weight);
        const float normal_d = dot(center.normal - sample.normal, center.normal - sample.normal);
        const float normal_w = exp(-normal_d / options.normal_weight);
        const float pos_d = dot(center.position - sample.position, center.position - sample.position);
        const float pos_w = exp(-pos_d / options.pos_weight);
        const float weight = color_w * normal_w * pos_w;

        /* Update sum & cummulative weight */
        sum += sample.luminance * weight;
        cum_weight += weight;
    }

    lbuffer_out[thread_id] = sum / cum_weight;
#endif

    /* Estimate pixel variance */
    // const uint64_t key = asuint64(vbuffer[thread_id]);
    // if (null_visibility(key)) { lbuffer_out[thread_id] = 0.0f; return; }
    // const cache::Slot slot = cache::fetch(macrofacet_cache, key);
    // if (slot.key == cache::EMPTY_KEY) return;
    // const float variance = abs(cs::acescg_luminance(slot.read_diffuse()) - cs::acescg_luminance(lbuffer_in[thread_id]));

    // const int radius = 1;  // 5x5 kernel
    // float2 sigmaVariancePair = float2(0.0f, 0.0f);
    // float sampCount = 0.0f;
    // for (int y = -radius; y <= radius; ++y) {
    //     for (int x = -radius; x <= radius; ++x) {
    //         int2 p = thread_id + int2(x, y);
    //         float3 curColor = lbuffer_in[p];
    //         float samp = cs::acescg_luminance(curColor);
    //         float sampSquared = samp * samp;
    //         sigmaVariancePair += float2(samp, sampSquared);
    //         sampCount += 1.0f;
    //     }
    // }
    // sigmaVariancePair /= sampCount;
    // float variance = max(0.0f, sigmaVariancePair.y - sigmaVariancePair.x * sigmaVariancePair.x);

    const Sample center = fetch_sample(thread_id);
    const float dist = distance(center.position, render_view.origin.xyz);
    float2 noise = r2(blue_noise[thread_id % 512].rg, render_view.frame_index % 1024u);
    float3 sum = 0.0f;
    float cum_weight = 0.0f;

    const float GoldenAngle = 2.399963229728f;

    /* Take 12 blue noise distributed neighbourhood samples */
    [ForceUnroll] for (uint i = 0u; i < 24u; ++i) {
        const float adapt_radius = options.sample_radius / dist; // * saturate(variance);

        float angle = GoldenAngle * (float)i;
        float radius = sqrt((float)i / 24.0f);
        const float2 p = float2(cos(angle), sin(angle)) * radius * adapt_radius;
        const int2 pixel = thread_id + int2(p);

        /* Sample neighbour */
        // const int2 offset = int2((r2(noise, i) * 2.0f - 1.0f) * options.sample_radius * saturate(variance));
        // const int2 pixel = thread_id + offset;
        const Sample sample = fetch_sample(pixel);
        if (dot(sample.normal, center.normal) <= 0.25f) continue;

        const float pos_d = max(0.001f, dot(center.position - sample.position, center.position - sample.position));
        const float pos_w = exp(-pos_d / 0.5f);
        const float weight = pos_w;

        /* Update sum & cummulative weight */
        sum += sample.luminance * weight;
        cum_weight += weight;
    }

    lbuffer_out[thread_id] = sum / cum_weight;


    // lbuffer_out[thread_id] = float3(variance, variance, variance);
}
