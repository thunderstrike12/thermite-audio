import view;
import particles;

struct VertexInput {
    float2 pos;
    float2 uv;
};

struct VertexOutput {
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
    uint32_t tex_index : TEXINDEX0;
};

ConstantBuffer<RenderView> gpu_view;

StructuredBuffer<Particle> particles;
StructuredBuffer<uint32_t> alive_list;

[shader("vertex")]
VertexOutput main(VertexInput input, uint32_t instance_id: SV_InstanceID) {
    VertexOutput output;

    Particle particle = particles[alive_list[instance_id]];

    /* 1. Transform to clip space  */
    float4 clip_pos = mul(gpu_view.world_to_clip, float4(particle.pos, 1.0f));

    /* 2. Transform to pixel space */
    float2 pixel_center = gpu_view.resolution * (0.5f * (clip_pos.xy / clip_pos.w) + 0.5f);

    /* 3. Get pixel size */
    float pixel_size = /* particle.size */ 64.0f / clip_pos.w;

    /* 4. Get pixel pos */
    float2 pixel_pos = pixel_center + float2(input.pos.x, -input.pos.y) * pixel_size;

    /* 5. Convert to ndc space */
    float2 ndc_pos = (pixel_pos / gpu_view.resolution) * 2.0f - 1.0f;

    /* 6. Convert to clip space */
    output.pos = float4(ndc_pos * clip_pos.w, clip_pos.z, clip_pos.w);
    output.uv = input.uv;
    output.tex_index = particle.tex_index;

    return output;
}