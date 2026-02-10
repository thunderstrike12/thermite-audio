module lights;

import view;
import colorspace;
import math;

public enum LightType : uint {
    SPHERE_LIGHT,
    SUN_LIGHT,
    SPOT_LIGHT,
    TUBE_LIGHT,
};

/* Universal light descriptor. */
public struct Light {
    float4 type_origin;
    float4 luminance_source;
    float4 attenuation_culling_beam_blend;
    float4 direction_length;

    /* The kind of light source. */
    public property LightType type { get { return (LightType)asuint(type_origin.x); } }
    /* Origin point of the light source. */
    public property float3 origin { get { return type_origin.yzw; } }
    /* Primary direction of the light source. */
    public property float3 direction { get { return direction_length.xyz; } }

    /* Exitent luminance of the light source. (in cd/m2, in ACEScg color-space) */
    public property float3 luminance { get { return luminance_source.xyz; } }

    /* Radius of the light source, defines the softness of its shadows. (cos(theta) for sun lights) */
    public property float source_radius { get { return luminance_source.w; } }
    /* Length of the light source. (only used for tube lights) */
    public property float source_length { get { return direction_length.w; } }

    /* Radius/distance at which the emitted light influence reaches zero. */
    public property float attenuation_radius { get { return attenuation_culling_beam_blend.x; } }
    /* The softness of the spot light edge. (0..1) */
    public property float spot_blend { get { return attenuation_culling_beam_blend.w; } }

    /* Radius/distance at which the light source should be culled. (used for coarse culling) */
    public property float culling_radius { get { return attenuation_culling_beam_blend.y; } }
    /* Cosine of the angular radius of the spot light beam. (also used for fine culling) */
    public property float cos_beam_angle { get { return attenuation_culling_beam_blend.z; } }
};

/* Calculate distance attenuation. */
public [ForceInline] float distance_attenuation(const float dist, const float radius) {
    const float relative_dist = saturate(dist / radius);
    const float inverse_dist = 1.0f - relative_dist * relative_dist;
    return (inverse_dist * inverse_dist) / (1.0f + 4.0f * relative_dist);
}

/* For a given sample point and normal, evaluate the weight of a light source. */
public [ForceInline] float evaluate_weight(const float3 p, const float3 n, const Light light) {
    if (all(light.luminance <= 0.0f)) return 0.0f;

    /* Calculate the local origin and distance to the light source */
    const float3 src_local = light.origin - p;
    const float src_dist = length(src_local);
    if (src_dist >= light.culling_radius || src_dist <= 0.0f) return 0.0f; /* Out of range */

    /* Calculate the direction and angle towards the light source */
    const float3 src_dir = src_local / src_dist;
    const float src_angle = dot(src_dir, -light.direction);
    if (src_angle < light.cos_beam_angle) return 0.0f; /* Out of range */

    /* Calculate the weight based on the perceived brightness of the light */
    const float luminance = cs::acescg_luminance(light.luminance);
    return 1.0f + luminance * distance_attenuation(src_dist, light.culling_radius);
}

/* Light source sample. */
public struct LightSample {
    public float3 dir;
    public float rcp_pdf;
    public float tmax;
};

/* Generate a sample for the sun light source. */
public [ForceInline] LightSample sample_sun(const float3 point, const SceneView scene_view, const float2 r01) {
    LightSample sample;

    /* Generate a direction within a cone in local-space */
    const float cos_theta = 1.0f - r01.x * (1.0f - scene_view.sun_angle);
    const float sin_theta = sqrt(1.0f - cos_theta * cos_theta);
    const float phi = 2.0f * PI_F32 * r01.y;
    const float3 local_dir = float3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);

    /* Form an orthonormal basis around the light source direction */
    const Basis basis = orthonormal_basis(scene_view.sun_dir);

    /* Convert the local-space direction to a world-space one */
    sample.dir = local_dir.x * basis.a + local_dir.y * basis.b + local_dir.z * scene_view.sun_dir;
    sample.tmax = BIG_F32;

    /* Calculate the PDF of the sample */
    sample.rcp_pdf = 2.0f * PI_F32 * (1.0f - scene_view.sun_angle);
    return sample;
}

/* Generate a sample for a universal light source. */
public [ForceInline] LightSample sample_light(const float3 point, const Light light, const float2 r01) {
    LightSample sample;

    /* Sphere, Sun, and Spot lights take the same path */
    const float3 light_dir = normalize(light.origin - point);
    const float light_dist = max(0.001f, distance(light.origin, point));
    const float sample_angle = sqrt(1.0f - pow(light.source_radius / light_dist, 2.0f));

    /* Tube light requires special path */
    if (light.type == LightType::TUBE_LIGHT) {
        /* Generate a random point on the tube */
        const float3 tube_point = light.origin 
            + (r01.x - 0.5f) * light.source_length * light.direction 
            + (r01.y - 0.5f) * light.source_radius * cross(light_dir, light.direction);

        /* Calculate the tmax and sample direction */
        const float3 sample_dir = tube_point - point;
        sample.tmax = max(0.001f, length(sample_dir));
        sample.dir = sample_dir / sample.tmax;

        /* Calculate the PDF of the sample */
        const float tube_area = light.source_length * light.source_radius;
        const float cos_light = abs(dot(light_dir, sample.dir));
        sample.rcp_pdf = (tube_area * cos_light) / (sample.tmax * sample.tmax);
        return sample;
    }

    /* Generate a direction within a cone in local-space */
    const float cos_theta = 1.0f - r01.x * (1.0f - sample_angle);
    const float sin_theta = sqrt(1.0f - cos_theta * cos_theta);
    const float phi = 2.0f * PI_F32 * r01.y;
    const float3 local_dir = float3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);

    /* Form an orthonormal basis around the light source direction */
    const Basis basis = orthonormal_basis(light_dir);

    /* Convert the local-space direction to a world-space one */
    sample.dir = local_dir.x * basis.a + local_dir.y * basis.b + local_dir.z * light_dir;

    /* Ray vs sphere intersection to find tmax */
    const float3 light_local = point - light.origin;
    const float b = dot(light_local, sample.dir);
    const float c = dot(light_local, light_local) - light.source_radius * light.source_radius;
    sample.tmax = max(0.0f, -b + sqrt(b * b - c));

    /* Calculate the PDF of the sample */
    sample.rcp_pdf = 2.0f * PI_F32 * (1.0f - sample_angle);
    return sample;
}
