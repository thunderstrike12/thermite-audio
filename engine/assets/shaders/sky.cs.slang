import view;
import math;
import bindless;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Active scene view. */
ConstantBuffer<SceneView> scene_view;
/* Environment map sampler. */
SamplerState envmap_sampler;
/* Depth buffer. */
Texture2D<float> dbuffer;
/* Luminance buffer. */
RWTexture2D<float3> lbuffer;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (any(thread_id >= render_view.resolution)) return;

    /* Do nothing on non-sky pixels */
    if (dbuffer[thread_id] < 1.0f) return;

    /* Add the sky HDRI onto the luminance */
    if (scene_view.envmap_full_handle != 0u) {
        const Ray ray = pinhole_ray(render_view, thread_id);
        const float3 sky = bindless::texture_sampled<Texture2D<float4>>(scene_view.envmap_full_handle).SampleLevel(envmap_sampler, dir_to_latlong(ray.dir), 0).rgb;
        lbuffer[thread_id] += sky;
    } else {
        /* Default environment color */
        lbuffer[thread_id] += float3(0.247f, 0.247f, 0.247f);
    }
}
