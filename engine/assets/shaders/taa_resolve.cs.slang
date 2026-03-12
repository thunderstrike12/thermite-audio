import colorspace;
import view;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Samplers. */
SamplerState point_sampler;
SamplerState linear_sampler;
/* Motion Vectors buffer. */
Texture2D<float2> mbuffer;
/* Luminance buffer. */
Texture2D<float3> lbuffer;
/* History buffers. */
RWTexture2D<float4> hbuffer1;
Texture2D<float4> hbuffer2;
/* Render target. */
RWTexture2D<float4> render_target;

[[vk::push_constant]]
uint32_t taa_flag;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    float3 curr_luminance = lbuffer[thread_id];

    float3 output = float3(0.0f, 0.0f, 0.0f);
    if (taa_flag == 1) {
        const float2 uv = (float2(thread_id) + 0.5f) / float2(render_view.resolution);

        const float2 uv_last = uv + mbuffer.SampleLevel(point_sampler, uv, 0.0f);

        float3 prev_luminance = hbuffer2.SampleLevel(linear_sampler, uv_last, 0.0f).rgb;

        /* Neighborhood clamp to prevent ghosting */
        float3 neighbour_min = curr_luminance, neighbour_max = curr_luminance;
        for (int y = -1; y <= 1; y++) {
            for (int x = -1; x <= 1; x++) {
                float3 s = lbuffer[thread_id + int2(x, y)];
                neighbour_min = min(neighbour_min, s);
                neighbour_max = max(neighbour_max, s);
            }
        }
        prev_luminance = clamp(prev_luminance, neighbour_min, neighbour_max);

        /* Reject off-screen */
        const bool valid = all(uv_last >= 0.0f) && all(uv_last <= 1.0f);
        const float blend = valid ? 0.95f : 0.0f;

        output = lerp(curr_luminance, prev_luminance, blend);
        hbuffer1[thread_id] = float4(output, 1.0f);

    } else {
        output = curr_luminance;
    }

    /* Convert from ACEScg to r709 */
    output = cs::acescg_to_r709(output);

    /* Apply AGX tonemapping */
    output = cs::agx(output);
    output = cs::agx_look(output);
    output = cs::agx_eotf(output);

    /* Output device transform */
    output = cs::delinearize(output);

    render_target[thread_id] = float4(output, 1.0f);
}
