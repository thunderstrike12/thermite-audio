Texture2D<float3> in;
RWTexture2D<float3> out;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    out[thread_id] = in[thread_id];
}
