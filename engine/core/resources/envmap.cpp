#include "envmap.hpp"

#include <stb_image.h>
#include <glm/gtc/packing.hpp>

#include <graphite/vram_bank.hh>

#include "engine/engine.hpp"

#include "engine/core/logger.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/shared/colorspace.hpp"

namespace tmt {

/* Convert spherical coordinates to a direction vector. */
static inline glm::vec3 spherical_to_dir(const float phi, const float theta) {
    return glm::vec3(sinf(theta) * sinf(phi), cosf(theta), sinf(theta) * cosf(phi));
}

/* Convert direction vector to spherical coordinates. */
static inline glm::vec2 dir_to_spherical(const glm::vec3& d) {
    const float phi = atan2(d.x, -d.z) + glm::pi<float>();
    const float theta = atan(d.y / sqrtf(d.x * d.x + d.z * d.z)) + glm::pi<float>() / 2.0f;
    return glm::vec2(glm::pi<float>() * 2.0f - phi, glm::pi<float>() - theta);
}

/* Number of uniform samples along each axis of the cones spherical coordinates. */
constexpr int CONE_SAMPLE_WIDTH = 128;
/* Maximum intensity of light from the environment map, helps avoid bright spots. */
constexpr float MAX_INTENSITY = 100000.0f;

/* Pre-filter an environment map for a specific cone aperture. (in radians) */
void conic_prefilter(const float* src, float* dst, int src_w, int src_h, int src_n, int dst_w, int dst_h, int dst_n, const float aperture) {
    const float sq_aperture = aperture * aperture;
    const float step_size = aperture * 2.0f / CONE_SAMPLE_WIDTH;

    /* Integrate every texel in the output envmap */
    for (int y = 0; y < dst_h; ++y) {
        for (int x = 0; x < dst_w; ++x) {
            const int dst_i = (x + y * dst_w) * dst_n;

            /* Get the spherical coordinates for this envmap texel */
            const float dst_phi = (((float)x + 0.5f) / dst_w) * glm::pi<float>() * 2.0f;
            const float dst_theta = (((float)y + 0.5f) / dst_h) * glm::pi<float>();

            /* Uniformly sample the cone for this texel */
            float r = 0.0f, g = 0.0f, b = 0.0f, s = 0.0f;
            for (int j = 0; j < CONE_SAMPLE_WIDTH; ++j) {
                for (int i = 0; i < CONE_SAMPLE_WIDTH; ++i) {
                    /* Get the spherical coordinates for this uniform sample */
                    const float src_phi = dst_phi - aperture + step_size * i;
                    const float src_theta = dst_theta - aperture + step_size * j;

                    /* Reject samples outside our cone aperture */
                    const float phi_diff = dst_phi - src_phi, theta_diff = dst_theta - src_theta;
                    if ((phi_diff * phi_diff + theta_diff * theta_diff) > sq_aperture) {
                        continue;
                    }

                    /* Make sure samples outside the range wrap around */
                    const glm::vec3 src_dir = spherical_to_dir(src_phi, src_theta);
                    const glm::vec2 scoords = dir_to_spherical(src_dir);

                    /* Convert the spherical coordinates to a source envmap texel */
                    const int src_x = (int)((scoords.x / (glm::pi<float>() * 2.0f)) * src_w);
                    const int src_y = (int)((scoords.y / glm::pi<float>()) * src_h);
                    const int src_i = (src_x + src_y * src_w) * src_n;

                    /* Sample the source envmap */
                    const float mag = sqrtf(src[src_i + 0] * src[src_i + 0] + src[src_i + 1] * src[src_i + 1] + src[src_i + 2] * src[src_i + 2]);
                    if (mag > MAX_INTENSITY) {
                        const float rcp_mag = (1.0f / mag) * MAX_INTENSITY;
                        r += src[src_i + 0] * rcp_mag;
                        g += src[src_i + 1] * rcp_mag;
                        b += src[src_i + 2] * rcp_mag;
                    } else {
                        r += src[src_i + 0];
                        g += src[src_i + 1];
                        b += src[src_i + 2];
                    }
                    s += 1.0f;
                }
            }

            /* Normalize and store the sampled results */
            const float rcp_s = 1.0f / s;
            dst[dst_i + 0] = r * rcp_s;
            dst[dst_i + 1] = g * rcp_s;
            dst[dst_i + 2] = b * rcp_s;
        }
    }
}

/* For a given aperture, what is the minimum required envmap resolution. */
void prefilter_resolution(int& out_w, int& out_h, const float aperture) {
    const float cone_angle = aperture * 2.0f;

    /* x4 to avoid aliasing */
    out_h = (int)ceilf(glm::pi<float>() / cone_angle * 4.0f);
    out_w = out_h * 2; /* <- width is always 2x height */
}

void pack_and_upload(VRAMBank& bank, Texture& texture, const float* data, const uint32_t w, const uint32_t h) {
    /* Pack floating point RGB into RGBA16 */
    uint64_t* packed_data = new uint64_t[w * h] {};
    for (uint32_t i = 0u; i < w * h; ++i) {
        const glm::vec3 unpacked = cs::r709_to_acescg(glm::vec3(data[i * 3u], data[i * 3u + 1u], data[i * 3u + 2u]));
        packed_data[i] = glm::packHalf4x16(glm::vec4(unpacked, 0.0f));
    }

    /* Upload the packed data */
    bank.upload_texture(texture, packed_data, w * h * sizeof(uint64_t)).expect("failed to upload envmap texture.");
    delete[] packed_data;
}

bool Envmap::load() {
    VRAMBank& bank = engine.renderer.vram_bank();
    int full_width = -1, full_height = -1, n = -1;

    /* Parse HDR data */
    float* data = stbi_loadf(file_location.get_absolute_path().string().c_str(), &full_width, &full_height, &n, 3);
    if (!data) return false;

    /* Save the image width, height, and name */
    width = (uint32_t)full_width;
    height = (uint32_t)full_height;
    name = file_location.get_relative_path().string();

    /* Calculate ideal resolution for filtered envmap */
    int filtered_width = -1, filtered_height = -1;
    prefilter_resolution(filtered_width, filtered_height, 0.3f);

    /* Perform filtering */
    // float* filtered_data = new float[filtered_width * filtered_height * 3] {};
    // conic_prefilter(data, filtered_data, full_width, full_height, 3, filtered_width, filtered_height, 3, 0.3f);

    { /* Init the filtered texture resource */
        const std::string texture_name = name + " Envmap (Filtered) Texture";
        filtered_texture =
            bank.create_texture(
                    texture_name.c_str(), TextureUsage::Sampled | TextureUsage::TransferDst, TextureFormat::RGBA16Sfloat, { (uint32_t)filtered_width, (uint32_t)filtered_height, 0u }
            )
                .expect("failed to initialise envmap texture.");
    }

    /* Pack and upload filtered texture */
    // pack_and_upload(bank, filtered_texture, filtered_data, filtered_width, filtered_height);
    // delete[] filtered_data;

    { /* Init the full texture resource */
        const std::string texture_name = name + " Envmap (Full) Texture";
        full_texture =
            bank.create_texture(texture_name.c_str(), TextureUsage::Sampled | TextureUsage::TransferDst, TextureFormat::RGBA16Sfloat, { (uint32_t)full_width, (uint32_t)full_height, 0u })
                .expect("failed to initialise envmap texture.");
    }

    /* Pack and upload full texture */
    pack_and_upload(bank, full_texture, data, full_width, full_height);
    stbi_image_free(data);

    /* Create image resources */
    const std::string full_image_name = name + " Envmap Image";
    full_image = bank.create_image(full_image_name.c_str(), full_texture).expect("failed to initialize envmap image.");
    const std::string filtered_image_name = name + " Envmap (Filtered) Image";
    filtered_image = bank.create_image(filtered_image_name.c_str(), filtered_texture).expect("failed to initialize envmap image.");

    return true;
}

void Envmap::unload() {
    auto& bank = engine.renderer.vram_bank();

    bank.destroy(full_texture);
    bank.destroy(full_image);
    bank.destroy(filtered_texture);
    bank.destroy(filtered_image);
}

}  // namespace tmt
