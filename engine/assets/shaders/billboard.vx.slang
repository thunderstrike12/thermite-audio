import view;
import particles;

struct VertexInput {
    float2 pos;
    float2 uv;
};

ConstantBuffer<RenderView> gpu_view;

StructuredBuffer<Particle> particles;
StructuredBuffer<uint32_t> alive_list;

[shader("vertex")]
BillboardVertexOutput main(VertexInput input, uint32_t instance_id: SV_InstanceID) {
    const Particle particle = particles[alive_list[instance_id]];

    /* 1. Transform to clip space  */
    const float4 clip_pos = mul(gpu_view.world_to_clip, float4(particle.pos, 1.0f));

    /* 2. Transform to pixel space */
    const float2 pixel_center = gpu_view.resolution * (0.5f * (clip_pos.xy / clip_pos.w) + 0.5f);

    /* 3. Get pixel size */
    const float pixel_size = particle.size / clip_pos.w;

    /* 3.5. Rotate the positions */
    const float c = cos(particle.rotation);
    const float s = sin(particle.rotation);
    const float2 rotated = float2(input.pos.x * c - input.pos.y * s, input.pos.x * s + input.pos.y * c);

    /* 4. Get pixel pos */
    const float2 pixel_pos = pixel_center + float2(rotated.x, -rotated.y) * pixel_size;

    /* 5. Convert to ndc space */
    const float2 ndc_pos = (pixel_pos / gpu_view.resolution) * 2.0f - 1.0f;

    BillboardVertexOutput output;
    /* 6. Convert to clip space */
    output.pos = float4(ndc_pos * clip_pos.w, clip_pos.z, clip_pos.w);

    /* Motion Vectors */
    float2 curr_ndc = clip_pos.xy / clip_pos.w;

    const float4 prev_clip_pos = mul(gpu_view.prev_world_to_clip, float4(particle.prev_pos, 1.0f));
    float2 prev_ndc = prev_clip_pos.xy / prev_clip_pos.w;

    curr_ndc -= gpu_view.jitter;
    prev_ndc -= gpu_view.prev_jitter;

    output.motion_vec = (prev_ndc - curr_ndc) * float2(0.5f, 0.5f);

    if (length(output.motion_vec) < 1e-5f)
        output.motion_vec = float2(0.0f, 0.0f);

    output.uv = input.uv;
    output.tex_index = particle.tex_index;
    output.opacity = particle.opacity;
    output.flipbook_frames = particle.flipbook_frames;
    output.current_frame = particle.current_frame;
    output.dither_scale = particle.dither_scale;

    return output;
}