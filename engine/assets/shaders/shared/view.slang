module view;

/* Represents a renderable view in the scene. */
public struct RenderView {
    /* World-space to clip-space transformation matrix. */
    public float4x4 world_to_clip;
    /* Clip-space to world-space transformation matrix. */
    public float4x4 clip_to_world;
    /* Origin of the view in world-space. */
    public float4 origin;
    /* Resolution of the view in pixels. */
    public uint2 resolution;
};

/* Represents a ray that can be traced. */
public struct Ray {
    public float3 origin;
    public float3 dir;
    public float3 rcp_dir;
};

/* Convert a pixel coordinate to a normalized device coordinate. */
public inline float2 pixel_to_ndc(int2 pixel, uint2 resolution) {
    const float2 ndc = ((float2(pixel) + 0.5f) / resolution) * 2.0f - 1.0f;
    return ndc;
}

/* Generate a ray for a pin hole camera. */
public inline Ray pinhole_ray(RenderView view, int2 pixel) {
    const float2 ndc = pixel_to_ndc(pixel, view.resolution);

    float4 world_pos = mul(view.clip_to_world, float4(ndc, 1.0f, 1.0f));
    world_pos.xyz /= world_pos.w;
    const float3 world_dir = normalize(world_pos.xyz - view.origin.xyz);

    Ray ray; /* Fill in the ray struct */
    ray.origin = view.origin.xyz;
    ray.dir = normalize(world_dir);
    ray.rcp_dir = rcp(ray.dir);
    return ray;
}
