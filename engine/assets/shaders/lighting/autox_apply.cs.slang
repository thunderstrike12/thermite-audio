import visibility;
import math;

/* Luminance buffer. */
RWTexture2D<float3> lbuffer;
Texture2D<float> autox_buffer;

[[vk::push_constant]]
float3 autox_settings;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    const float3 luminance = lbuffer[thread_id];
    const float exposure = autox_settings.x / clamp(autox_buffer[0], autox_settings.y, autox_settings.z);
    lbuffer[thread_id] = luminance * exposure;
}
