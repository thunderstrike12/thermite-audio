import colorspace;

Texture2D<float3> lbuffer;
RWTexture2D<float3> tbuffer;

[[vk::push_constant]]
float2 bloom_settings;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(uint2 thread_id: SV_DispatchThreadID) {
    const float3 luminance = lbuffer[thread_id];

    const float brightness = cs::acescg_luminance(luminance);

    if (brightness > bloom_settings.x) {
        tbuffer[thread_id] = luminance;
        return;
    }

    const float trail = 1.0f - brightness / bloom_settings.x;
    const float t = saturate(trail / bloom_settings.y);

    tbuffer[thread_id] = lerp(luminance, 0.0f, t); // float3(0.0f, 0.0f, 0.0f);
}
