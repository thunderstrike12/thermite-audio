import colorspace;
import visibility;
import math;

/* Luminance buffer. */
Texture2D<float3> lbuffer;
Texture2D<float3> tbuffer;
/* Render Target. */
RWTexture2D<float4> render_target;

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

    render_target[thread_id] = float4(luminance, 1.0f);
}
