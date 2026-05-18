module view;

import math;

/* Represents a renderable view in the scene. */
public struct RenderView {
    /* World-space to clip-space transformation matrix. */
    public float4x4 world_to_clip; 
    /* Prev frame world-space to clip-space transformation matrix. */
    public float4x4 prev_world_to_clip;
    /* Clip-space to world-space transformation matrix. */
    public float4x4 clip_to_world;
    /* Origin of the view in world-space. */
    public float4 origin;
    /* Previous origin of the view in world-space. */
    public float4 prev_origin;
    /* Resolution of the view in pixels. */
    public uint2 resolution;
    /* Index of the current frame. */
    public uint frame_index;
    /* Delta Time in seconds. */
    public float dt;
    /* 0 = FullRate, 1 = HalfRate, 2 = QuarterRate */
    public uint diff_shading_rate;
    /* 0 = FullRate, 1 = HalfRate, 2 = QuarterRate */
    public uint spec_shading_rate;
    /* Current frame jitter offset */
    public float2 jitter;
    /* Prev frame jitter offset */
    public float2 prev_jitter;
};

/* Holds information about the active scene. */
public struct SceneView {
    /* Direction pointing towards the sun. */
    public float3 sun_dir;
    /* Radius as cos(theta) of the sun, defines the softness of its shadows. */
    public float sun_angle;
    /* Exitent luminance of the light source. (in cd/m2, in ACEScg color-space) */
    public float3 sun_luminance;
    /* Number of lights present in the scene. */
    public uint light_count;
    /* Environment map (full) bindless handle. */
    public uint envmap_full_handle;
    /* Environment map (filtered) bindless handle. */
    public uint envmap_filtered_handle;
    /* Color grading lut bindless handle. */
    public uint lut_handle;
};

/* Represents a ray that can be traced. */
public struct Ray {
    public float3 origin;
    public float3 dir;
    public float3 rcp_dir;
};

/* Represents a hit resulted from tracing a ray. */
public struct Hit {
    public float t;
    public uint voxel_index;
    public uint object_index;
    public uint face_index;
    public uint steps;

    public inline static Hit miss() {
        Hit hit;
        hit.t = BIG_F32;
        hit.steps = 0u;
        return hit;
    }
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
    ray.dir = world_dir;
    ray.rcp_dir = clamp(rcp(ray.dir), -1e30f, 1e30f);
    return ray;
}

/* Generate a custom ray. */
public inline Ray custom_ray(float3 origin, float3 dir) {
    Ray ray; /* Fill in the ray struct */
    ray.origin = origin;
    ray.dir = dir;
    ray.rcp_dir = clamp(rcp(ray.dir), -1e30f, 1e30f);
    return ray;
}
