import colorspace;
import view;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Samplers. */
SamplerState point_sampler;
SamplerState linear_sampler;
/* Motion Vectors buffer. */
Texture2D<float2> mbuffer;
/* Depth buffer. */
Texture2D<float> dbuffer;
/* Current stencil buffer. */
[vk::image_format("unknown")]
Texture2D<uint32_t> stencil_buffer;
/* Previous frame stencil buffer. */
[vk::image_format("unknown")]
Texture2D<uint32_t> prev_stencil_buffer;
/* Luminance buffer. */
RWTexture2D<float3> lbuffer;
/* History buffers. */
RWTexture2D<float4> hbuffer1;
Texture2D<float4> hbuffer2;

[[vk::push_constant]]
uint32_t taa_flag;

float3 clip_aabb(float3 aabb_min, float3 aabb_max, float3 history) {
    const float3 center = 0.5f * (aabb_max + aabb_min);
    const float3 extents = 0.5f * (aabb_max - aabb_min) + 1e-5f;

    const float3 offset = history - center;
    const float3 ts = abs(extents / offset);
    const float t = saturate(min(ts.x, min(ts.y, ts.z)));

    return center + offset * t;
}

// Source: TheRealMJP - https://gist.github.com/TheRealMJP/c83b8c0f46b63f3a88a5986f4fa982b1
// Samples a texture with Catmull-Rom filtering, using 9 texture fetches instead of 16.
// See http://vec3.ca/bicubic-filtering-in-fewer-taps/ for more details
float4 sample_texture_catmull_roll(in Texture2D<float4> tex, in SamplerState linearSampler, in float2 uv, in float2 texSize)
{
    // We're going to sample a a 4x4 grid of texels surrounding the target UV coordinate. We'll do this by rounding
    // down the sample location to get the exact center of our "starting" texel. The starting texel will be at
    // location [1, 1] in the grid, where [0, 0] is the top left corner.
    float2 samplePos = uv * texSize;
    float2 texPos1 = floor(samplePos - 0.5f) + 0.5f;

    // Compute the fractional offset from our starting texel to our original sample location, which we'll
    // feed into the Catmull-Rom spline function to get our filter weights.
    float2 f = samplePos - texPos1;

    // Compute the Catmull-Rom weights using the fractional offset that we calculated earlier.
    // These equations are pre-expanded based on our knowledge of where the texels will be located,
    // which lets us avoid having to evaluate a piece-wise function.
    float2 w0 = f * (-0.5f + f * (1.0f - 0.5f * f));
    float2 w1 = 1.0f + f * f * (-2.5f + 1.5f * f);
    float2 w2 = f * (0.5f + f * (2.0f - 1.5f * f));
    float2 w3 = f * f * (-0.5f + 0.5f * f);

    // Work out weighting factors and sampling offsets that will let us use bilinear filtering to
    // simultaneously evaluate the middle 2 samples from the 4x4 grid.
    float2 w12 = w1 + w2;
    float2 offset12 = w2 / w12;

    // Compute the final UV coordinates we'll use for sampling the texture
    float2 texPos0 = texPos1 - 1;
    float2 texPos3 = texPos1 + 2;
    float2 texPos12 = texPos1 + offset12;

    texPos0 /= texSize;
    texPos3 /= texSize;
    texPos12 /= texSize;

    float4 result = 0.0f;
    result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos0.y), 0.0f) * w0.x * w0.y;
    result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos0.y), 0.0f) * w12.x * w0.y;
    result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos0.y), 0.0f) * w3.x * w0.y;

    result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos12.y), 0.0f) * w0.x * w12.y;
    result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos12.y), 0.0f) * w12.x * w12.y;
    result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos12.y), 0.0f) * w3.x * w12.y;

    result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos3.y), 0.0f) * w0.x * w3.y;
    result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos3.y), 0.0f) * w12.x * w3.y;
    result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos3.y), 0.0f) * w3.x * w3.y;

    return result;
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    float3 curr_luminance = lbuffer[thread_id];

    if (taa_flag != 1u) {
        lbuffer[thread_id] = curr_luminance;
        return;
    }

    float3 output = float3(0.0f, 0.0f, 0.0f);

    /* Neighborhood clamp to prevent ghosting */
    /* m1 & m2 are used for variance clipping */
    float3 m1 = float3(0.0f, 0.0f, 0.0f);
    float3 m2 = float3(0.0f, 0.0f, 0.0f);
    float3 neighbour_min = curr_luminance;
    float3 neighbour_max = curr_luminance;
    int2 closest_offset = int2(0, 0);
    float closest_depth = 0.0f;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            const float3 s = lbuffer[thread_id + int2(x, y)];
            m1 += s;
            m2 += s * s;
            neighbour_min = min(neighbour_min, s);
            neighbour_max = max(neighbour_max, s);

            const float d = dbuffer[thread_id + int2(x, y)];
            if (d > closest_depth) {
                closest_depth = d;
                closest_offset = int2(x, y);
            }
        }
    }

    /* Check for stenciled particles */
    const uint32_t curr_stencil = stencil_buffer[thread_id];

    /* Variance clipping (build tighter aabb) */
    /* https://alextardif.com/TAA.html */
    const float3 mu = m1 / 9.0f;
    const float3 sigma = sqrt(abs(m2 / 9.0f - mu * mu));
    const float gamma = curr_stencil == 1u ? 2.0f : 1.0f; /* Allow higher color range for particles (widden the aabb) */
    const float3 var_min = max(mu - gamma * sigma, neighbour_min);
    const float3 var_max = min(mu + gamma * sigma, neighbour_max);

    /* Reproject */
    const float2 uv = (float2(thread_id) + 0.5f) / float2(render_view.resolution);
    const float2 uv_last = uv + mbuffer[thread_id + closest_offset];

    /* Sample and clip history */
    float3 prev_luminance = sample_texture_catmull_roll(hbuffer2, linear_sampler, uv_last, float2(render_view.resolution)).rgb;
    prev_luminance = clip_aabb(var_min, var_max, prev_luminance);

    /* Reject off-screen */
    const bool valid = all(uv_last >= 0.0f) && all(uv_last <= 1.0f);
    float blend = valid ? 0.95f : 0.0f;

    /* Use less of previous frame if we reproject particles */
    const uint32_t prev_stencil = prev_stencil_buffer.SampleLevel(point_sampler, uv_last, 0.0f);
    blend = curr_stencil == prev_stencil ? blend : 0.1f;
            
    output = lerp(curr_luminance, prev_luminance, blend);

    /* NaN rejection check */
    if (any(isnan(output)) == false) {
        hbuffer1[thread_id] = float4(output, 1.0f);
        lbuffer[thread_id] = output;
    }
}
