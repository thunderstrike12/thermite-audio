import colorspace;

Texture2D<float3> lbuffer;
RWTexture2D<float3> tbuffer;

[[vk::push_constant]]
float bloom_threshold;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(uint2 thread_id: SV_DispatchThreadID) {
    const float3 luminance = lbuffer[thread_id];

    const float brightness = cs::acescg_luminance(luminance);

    if (brightness > bloom_threshold) tbuffer[thread_id] = luminance;
    else tbuffer[thread_id] = float3(0.0f, 0.0f, 0.0f);

    return;
}