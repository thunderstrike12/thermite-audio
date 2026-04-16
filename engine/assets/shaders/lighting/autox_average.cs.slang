/**
 * @name AutoX Average [128x128 threads]
 * This pass downsamples the partial luminance from 256x256 texels to 16x16 texels for auto-exposure.
 */
import view;

/* 256x256 Luminance buffer. */
Texture2D<float> partial_lbuffer;
/* 16x16 Luminance buffer. */
RWTexture2D<float> tiny_lbuffer;
/* Down Sampler */
SamplerState down_sampler;

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
    /* Use a bilinear sampler to down sample from 256x256 to 128x128 */
    const float2 uv = (float2(thread_id) + 0.5f) / 128.0f;
    float avg_lum = partial_lbuffer.SampleLevel(down_sampler, uv, 0);
    lds_store_lum(group_i, avg_lum);
    GroupMemoryBarrierWithGroupSync();

    /* Mask discards any non multiple of 2 threads. */
    if ((group_i & 0b001001) == 0) {
        const float lum1 = lds_load_lum(group_i + 0x01);
        const float lum2 = lds_load_lum(group_i + 0x08);
        const float lum3 = lds_load_lum(group_i + 0x09);
        avg_lum = 0.25f * (avg_lum + lum1 + lum2 + lum3);
        lds_store_lum(group_i, avg_lum);
    }
    GroupMemoryBarrierWithGroupSync();

    /* Mask discards any non multiple of 4 threads. */
    if ((group_i & 0b011011) == 0) {
        const float lum1 = lds_load_lum(group_i + 0x02);
        const float lum2 = lds_load_lum(group_i + 0x10);
        const float lum3 = lds_load_lum(group_i + 0x12);
        avg_lum = 0.25f * (avg_lum + lum1 + lum2 + lum3);
        lds_store_lum(group_i, avg_lum);
    }
    GroupMemoryBarrierWithGroupSync();

    /* Mask discards all but 1 thread, which is thread 0. */
    if (group_i == 0) {
        const float lum1 = lds_load_lum(group_i + 0x04);
        const float lum2 = lds_load_lum(group_i + 0x20);
        const float lum3 = lds_load_lum(group_i + 0x24);
        avg_lum = 0.25f * (avg_lum + lum1 + lum2 + lum3);

        /* Finally write the 16x16 downsampled luminance into a tiny luminance buffer */
        tiny_lbuffer[thread_id >> 3] = avg_lum; /* thread_id / 8 */
    }
}
