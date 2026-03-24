import view;
import visibility;
import math;
import voxel;
import cache;
import colorspace;

/* Active render view */
ConstantBuffer<RenderView> render_view;
/* Visibility buffer */
//Texture2D<uint2> vbuffer;
///* Macrofacet illuminance cache buffer. */
//RWStructuredBuffer<cache::Slot> macrofacet_cache;
/* Luminance buffer. */
Texture2D<float3> lbuffer;
/* Render target. (Output) */
RWTexture2D<float4> render_target;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    int2 pixel_id = thread_id;
    // if (render_view.shading_rate == 1u) {
    //     /* Half-rate */
    //     /*  [ XY ]  */
    //     /*  [ YX ]  */
    //     const int offset = (pixel_id.y + render_view.frame_index) & 0b1;
    //     pixel_id.x = (pixel_id.x + offset) >> 1;
    // } else if (render_view.shading_rate == 2u) {
    //     /* Quarter-rate */
    //     /*  [ XY ]  */
    //     /*  [ ZW ]  */
    //     const int2 offset = int2(render_view.frame_index & 0b1, (render_view.frame_index >> 1) & 0b1);
    //     pixel_id = (pixel_id + offset) >> 1;
    // }

    /* Bounds check */
    if (any(pixel_id >= render_view.resolution)) return;

    /* Load visibility buffer data */
    //const uint64_t packed = asuint64(vbuffer[thread_id]);

    //if (null_visibility(packed)) {
    //    render_target[thread_id] = float4(0.075f, 0.075f, 0.075f, 1.0f);
    //} else {
    //    const cache::Slot slot = cache::fetch(macrofacet_cache, packed);

    //    if (slot.key == cache::EMPTY_KEY) {
    //        render_target[pixel_id] = float4(1.0f, 0.0f, 1.0f, 1.0f);
    //    } else {
    //        render_target[pixel_id] = float4(cs::delinearize(cs::acescg_to_r709(cs::aces_filmic_tonemapping(slot.read_diffuse()))), 1.0f);
    //    }
    //}

    /* Convert from ACEScg to r709 */
    float3 luminance = lbuffer[pixel_id];
    luminance = cs::acescg_to_r709(luminance);

    /* Output device transform */
    luminance = cs::delinearize(luminance);

    /* Render luminance to the screen */
    render_target[thread_id] = float4(luminance, 1.0f); /* TEMP: Remove tonemapping! */
    // render_target[pixel_id] = float4(cs::delinearize(cs::acescg_to_r709(cs::aces_filmic_tonemapping(slot.read_diffuse()))), 1.0f);
}
