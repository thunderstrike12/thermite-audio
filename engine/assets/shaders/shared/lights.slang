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
    public float4 type_origin;
    public float4 luminance_source;
    public float4 attenuation_culling_beam_blend;
    public float4 direction_length;

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

/* Perform fine culling for a light source against a sphere. */
public [ForceInline] bool fine_cull_light(const Light light, const float3 c, const float r) {
    const float3 v = c - light.origin;
    const float dist_sq = dot(v, v);
    const float max_dist = light.culling_radius + r;

    /* Cull radius early out */
    if (dist_sq > max_dist * max_dist) return false;

    /* Cull beam angle for spot lights */
    if (light.type == LightType::SPOT_LIGHT) {
        const float sin_beam_angle = sqrt(1.0 - light.cos_beam_angle * light.cos_beam_angle);
        const float vdotd = dot(v, light.direction);

        if (vdotd < 0.0f) {
            /* Behind apex, only intersects if sphere contains the apex */
            return dist_sq <= r * r;
        }

        /* Calculate signed distance from sphere center to cone */
        const float perp_dist = sqrt(dist_sq - vdotd * vdotd);
        const float cone_dist = light.cos_beam_angle * perp_dist - sin_beam_angle * vdotd;
        if (cone_dist > r) return false;
    }
    return true;
}

/* Light source sample. */
public struct LightSample {
    public float3 dir;
    public float rcp_pdf;
    public float tmax;
    public uint index;
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
    const float3x3 basis = orthonormal_basis(scene_view.sun_dir);

    /* Convert the local-space direction to a world-space one */
    sample.dir = local_dir.x * basis[0] + local_dir.y * basis[1] + local_dir.z * scene_view.sun_dir;
    sample.tmax = BIG_F32;

    /* Calculate the PDF of the sample */
    sample.rcp_pdf = 2.0f * PI_F32 * (1.0f - scene_view.sun_angle);
    return sample;
}

/* Generate a sample for a universal light source. */
public [ForceInline] LightSample sample_light(const float3 point, const Light light, const float2 r01) {
    LightSample sample;

    /* Tube light requires special path */
    float3 light_origin = light.origin;
    if (light.type == LightType::TUBE_LIGHT) {
        /* Generate a random point on the tube */
        const float r = frac(r01.x + 1.61803398875f);
        light_origin = light.origin + (r - 0.5f) * light.source_length * light.direction;
    }

    /* Sphere, Sun, and Spot lights take the same path */
    const float3 light_dir = normalize(light_origin - point);
    const float light_dist = max(0.001f, distance(light_origin, point));
    const float sample_angle = sqrt(1.0f - pow(light.source_radius / light_dist, 2.0f));

    /* Handle samples inside the light source radius */
    if (light_dist <= light.source_radius) {
        sample.tmax = 0.0f;
        sample.rcp_pdf = 1.0f;
        return sample;
    }

    /* Generate a direction within a cone in local-space */
    const float cos_theta = 1.0f - r01.x * (1.0f - sample_angle);
    const float sin_theta = sqrt(1.0f - cos_theta * cos_theta);
    const float phi = 2.0f * PI_F32 * r01.y;
    const float3 local_dir = float3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);

    /* Form an orthonormal basis around the light source direction */
    const float3x3 basis = orthonormal_basis(light_dir);

    /* Convert the local-space direction to a world-space one */
    sample.dir = local_dir.x * basis[0] + local_dir.y * basis[1] + local_dir.z * light_dir;

    /* Ray vs sphere intersection to find tmax */
    const float3 light_local = point - light_origin;
    const float b = dot(light_local, sample.dir);
    const float c = dot(light_local, light_local) - light.source_radius * light.source_radius;
    sample.tmax = max(-b + sqrt(b * b - c), light_dist - light.source_radius);

    /* Calculate the PDF of the sample */
    sample.rcp_pdf = 2.0f * PI_F32 * (1.0f - sample_angle);
    return sample;
}

/* Calculate attenuation for a light and light sample at a given point. */
public [ForceInline] float attenuate_light(const float3 point, const Light light, const LightSample sample) {
    if (light.attenuation_radius <= 0.0f) return 0.0f;

    /* Distance based attenuation */
    const float relative_dist = saturate(sample.tmax / light.attenuation_radius);
    const float inverse_dist = 1.0f - relative_dist * relative_dist;
    float attenuation = (inverse_dist * inverse_dist) / (1.0f + 4.0f * relative_dist);

    /* Spot based attenuation */
    if (light.type == LightType::SPOT_LIGHT) {
        const float3 light_dir = normalize(point - light.origin);
        const float cos_theta = dot(light_dir, light.direction);
        const float cos_inner = lerp(light.cos_beam_angle, 1.0f, light.spot_blend * 2.0f);
        const float spot = saturate((cos_theta - light.cos_beam_angle) / (cos_inner - light.cos_beam_angle));
        attenuation *= spot * spot;
    }

    return attenuation;
}
