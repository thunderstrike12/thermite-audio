/* Motion vectors buffer. (Input) */
Texture2D<float2> mbuffer;
/* Render target. (Output) */
RWTexture2D<float4> render_target;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    float2 mv = mbuffer[thread_id];

    float2 output = (mv * 100.0f) * 0.5f + 0.5f;

    if (length(mv) == 0.0f)
        output = float2(0.0f, 0.0f);

    render_target[thread_id] = float4(output, 0.0f, 1.0f);
}
