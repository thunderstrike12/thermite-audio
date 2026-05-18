import colorspace;
import visibility;
import math;
import view;
import bindless;

/* Active scene view. */
ConstantBuffer<SceneView> scene_view;
/* Luminance buffer. */
Texture2D<float3> lbuffer;
/* Bloom buffer. */
Texture2D<float3> tbuffer;
/* Render Target. */
RWTexture2D<float4> render_target;
/* Color grading LUT 3D Sampler */
SamplerState lut_sampler;

float3 hue2rgb(float hue) {
    hue = fract(hue);
    return saturate(float3(abs(hue * 6.0f - 3.0f) - 1.0f, 2.0f - abs(hue * 6.0f - 2.0f), 2.0f - abs(hue * 6.0f - 4.0f)));
}

float3 rgb2hsl(float3 color) {
    const float color_min = min(min(color.r, color.g), color.b);
    const float color_max = max(max(color.r, color.g), color.b);
    const float delta = color_max - color_min;

    float3 hsl = float3(0.0f, 0.0f, (color_max + color_min) / 2.0f);
    if (delta != 0.0f) {                                    /* If it has chroma and isn't gray */
        if (hsl.z < 0.5f) {
            hsl.y = delta / (color_max + color_min);        /* Saturation */
        } else {
            hsl.y = delta / (2.0f - color_max - color_min); /* Saturation */
        }

        float delta_r = (((color_max - color.r) / 6.0f) + (delta / 2.0f)) / delta;
        float delta_g = (((color_max - color.g) / 6.0f) + (delta / 2.0f)) / delta;
        float delta_b = (((color_max - color.b) / 6.0f) + (delta / 2.0f)) / delta;

        // Hue.
        if (color.r == color_max) {
            hsl.x = delta_b - delta_g;
        } else if (color.g == color_max) {
            hsl.x = (1.0f / 3.0f) + delta_r - delta_b;
        } else {  // if(c.b==cMax){
            hsl.x = (2.0f / 3.0f) + delta_g - delta_r;
        }
        hsl.x = fract(hsl.x);
    }
    return hsl;
}

float3 hsl2rgb(float3 hsl) {
    if (hsl.y == 0.0f) {
        return float3(hsl.z); /* lightness */
    } else {
        float b;
        if (hsl.z < 0.5f) {
            b = hsl.z * (1.0f + hsl.y);
        } else {
            b = hsl.z + hsl.y - hsl.y * hsl.z;
        }
        float a = 2.0f * hsl.z - b;
        return a + hue2rgb(hsl.x) * (b - a);
    }
}

[[vk::push_constant]]
float lut_size;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    float3 luminance = lbuffer[thread_id];
    luminance += tbuffer[thread_id];

    /* Convert from ACEScg to r709 */
    luminance = cs::acescg_to_r709(luminance);

    /* Apply AGX tonemapping */
    luminance = cs::agx(luminance);
    luminance = cs::agx_look(luminance);
    luminance = cs::agx_eotf(luminance);

    /* Output device transform */
    luminance = cs::delinearize(luminance);

    /* Apply LUT Color Grading */
    if (scene_view.lut_handle != 0u) {
        const float scale = (lut_size - 1.0f) / lut_size;
        const float offset = 0.5f / lut_size;
        const float3 uvw = saturate(luminance) * scale + offset;

        luminance = bindless::texture_sampled<Texture3D<float4>>(scene_view.lut_handle).SampleLevel(lut_sampler, uvw, 0).rgb;
    }

    render_target[thread_id] = float4(luminance, 1.0f);
}
