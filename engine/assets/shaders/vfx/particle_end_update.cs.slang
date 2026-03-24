import particles;

RWStructuredBuffer<Counters> counters;
RWStructuredBuffer<DrawArgs> draw_args;

[shader("compute")]
[numthreads(1, 1, 1)]
void main(uint3 thread_id: SV_DispatchThreadID) {
    uint32_t particle_count = counters[0].draw_count;

    DrawArgs args;
    args.vertex_count = 6;
    args.instance_count = particle_count;
    args.first_vertex = 0;
    args.first_instance = 0;

    draw_args[0] = args;
}