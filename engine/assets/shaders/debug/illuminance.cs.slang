import view;
import visibility;
import math;
import voxel;
import cache;
import colorspace;

/* Active render view */
ConstantBuffer<RenderView> render_view;
/* Diffuse buffer. */
Texture2D<float3> diffuse_buffer;
/* Render target. (Output) */
RWTexture2D<float4> render_target;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (any(thread_id >= render_view.resolution)) return;

    float3 luminance = diffuse_buffer[full_to_rate(thread_id, render_view.diff_shading_rate)];
    luminance = cs::acescg_to_r709(luminance);
    luminance = cs::delinearize(luminance);
    render_target[thread_id] = float4(luminance, 1.0f);
}
