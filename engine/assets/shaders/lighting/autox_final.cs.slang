/**
 * @name AutoX Final [1 thread]
 * This pass downsamples the tiny luminance from 16x16 texels to an average luminance for auto-exposure.
 */
import view;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* 16x16 Luminance buffer. */
Texture2D<float> tiny_lbuffer;
/* 1x1 Auto-exposure buffer. */
RWTexture2D<float> autox_buffer;
/* Down Sampler */
SamplerState down_sampler;

[[vk::push_constant]]
float autox_response;

/* 8x8 luminance values in LDS. */
groupshared float gs_l[64];

/* Store luminance value into LDS. */
[ForceInline] void lds_store_lum(const uint i, const float lum) {
    gs_l[i] = lum;
}

/* Load luminance value from LDS. */
[ForceInline] float lds_load_lum(const uint i) {
    return gs_l[i];
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID, int group_i: SV_GroupIndex) {
    { /* Use a bilinear sampler to down sample from 16x16 to 8x8 */
        const float2 uv = (float2(thread_id) + 0.5f) / 8.0f;
        const float lum = tiny_lbuffer.SampleLevel(down_sampler, uv, 0);
        const float2 ndc = uv * 2.0f - 1.0f;
        const float w = (1.0f / 64.0f) * (1.0f - dot(ndc, ndc)); /* Center weight */

        lds_store_lum(group_i, lum * w);
        GroupMemoryBarrierWithGroupSync();
    }

    /* After the initial downsample, only allow 1 thread to continue */
    if (group_i != 0) return;

    /* Sum up all the 8x8 luminance values */
    /* (they are pre-weighted in the previous step) */
    float lum_sum = 0.0f;
    for (uint i = 0u; i < 64u; ++i) {
        lum_sum += lds_load_lum(i);
    }

    /* Finally update the auto exposure buffer */
    const float autox = autox_buffer[0];
    autox_buffer[0] = autox + (lum_sum - autox) * (1.0f - exp(-render_view.dt * autox_response));
}
