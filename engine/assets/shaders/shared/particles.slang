module particles;

public struct ParticleEmitter {
    public float3 pos;
    public float lifetime;

    public float3 dir;
    public float cone_angle;

    public float min_speed;
    public float max_speed;
    public uint32_t spawn_count;
    public uint32_t tex_index;
};

public struct Particle {
    public float3 pos;
    public float age;

    public float3 velocity;
    public float lifetime;

    public uint32_t tex_index;
};

public struct Counters {
    public uint32_t alive_count;
    public uint32_t alive_count_after_sim;
    public uint32_t dead_count;
    public uint32_t draw_count;
};

public struct DrawArgs {
    public uint32_t vertex_count;
    public uint32_t instance_count;
    public uint32_t first_vertex;
    public uint32_t first_instance;
};

public struct DispatchArgs {
    public uint32_t x;
    public uint32_t y;
    public uint32_t z;
};