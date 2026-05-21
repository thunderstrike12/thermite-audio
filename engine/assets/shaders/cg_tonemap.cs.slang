import colorspace;
import visibility;
import math;
import view;
import bindless;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Active scene view. */
ConstantBuffer<SceneView> scene_view;
/* Luminance buffer. */
Texture2D<float3> lbuffer;
/* Bloom buffer. */
Texture2D<float3> tbuffer;
/* Intermediate Target. */
RWTexture2D<float4> post_tonemap;
/* Color grading LUT 3D Sampler */
SamplerState lut_sampler;

[[vk::push_constant]]
float lut_size;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    float3 luminance = lbuffer[thread_id];
    luminance += tbuffer[thread_id];

    /* Convert from ACEScg to r709 */
    luminance = cs::acescg_to_r709(luminance);

    /* Apply AGX tonemapping */
    luminance = cs::agx(luminance);
    luminance = cs::agx_look(luminance);
    luminance = cs::agx_eotf(luminance);

    /* Output device transform */
    luminance = cs::delinearize(luminance);

    /* Apply LUT Color Grading */
    if (scene_view.lut_handle != 0u) {
        const float scale = (lut_size - 1.0f) / lut_size;
        const float offset = 0.5f / lut_size;
        const float3 uvw = saturate(luminance) * scale + offset;

        luminance = bindless::texture_sampled<Texture3D<float4>>(scene_view.lut_handle).SampleLevel(lut_sampler, uvw, 0).rgb;
    }

    post_tonemap[thread_id] = float4(luminance, 1.0f);
}
