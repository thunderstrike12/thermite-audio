/**
 * @name AutoX Gather [1 thread per texel]
 * This pass downsamples output luminance to 256x256 texels for auto-exposure.
 */
import view;
import colorspace;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Full-res Luminance buffer. */
Texture2D<float3> full_lbuffer;
/* 256x256 Luminance buffer. */
RWTexture2D<float> partial_lbuffer;

[shader("compute")]
[numthreads(8, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Grab the full output resolution and find the scale factor */
    const uint2 full_res = render_view.resolution;
    const float2 scale_factor = float2(full_res) / 256.0f;

    /* Calculate the pixel id to down-sample */
    const uint2 pixel_id = uint2(float2(thread_id) * scale_factor);
    partial_lbuffer[thread_id] = cs::acescg_luminance(full_lbuffer[pixel_id]);
}
