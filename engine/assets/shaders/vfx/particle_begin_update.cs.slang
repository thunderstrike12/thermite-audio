import particles;

RWStructuredBuffer<Counters> counters;
RWStructuredBuffer<DispatchArgs> dispatch_args;

[shader("compute")]
[numthreads(1, 1, 1)]
void main() {
    uint32_t alive_counter = counters[0].alive_count_after_sim;
    uint32_t dead_counter = counters[0].dead_count;

    DispatchArgs args;
    args.x = (alive_counter + 256 - 1) / 256;
    args.y = 1;
    args.z = 1;
    dispatch_args[0] = args;

    counters[0].alive_count = alive_counter;
    counters[0].alive_count_after_sim = 0;

    counters[0].dead_count = max(0, dead_counter);
    counters[0].draw_count = 0;
}