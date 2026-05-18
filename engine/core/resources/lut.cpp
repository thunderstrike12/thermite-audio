#include "lut.hpp"

#include <graphite/vram_bank.hh>

#include "engine.hpp"

#include "core/logger.hpp"
#include "core/renderer/renderer.hpp"

namespace tmt {

static uint16_t float_to_half(float f) {
    const uint32_t x = std::bit_cast<uint32_t>(f);
    const uint32_t sign = (x >> 16) & 0x8000u;
    const int32_t exp = int32_t((x >> 23) & 0xffu) - 127 + 15;
    const uint32_t mant = (x >> 13) & 0x3ffu;

    if (exp <= 0) return uint16_t(sign);               // underflow / denorm -> ±0
    if (exp >= 0x1f) return uint16_t(sign | 0x7c00u);  // overflow -> ±inf
    return uint16_t(sign | (uint32_t(exp) << 10) | mant);
}

bool LUT::load() {
    const std::string path_str = file_location.get_absolute_path().string();
    const std::string cube_text = IO::read_text_file(file_location);

    if (cube_text.empty()) {
        Log::error("LUT '{}' is empty or could not be read.", path_str);
        return false;
    }

    std::istringstream stream(cube_text);
    std::string line;

    int size = 0;
    std::vector<glm::vec3> data = {};

    try {
        while (std::getline(stream, line)) {
            const size_t first = line.find_first_not_of(" \t\r");
            if (first == std::string::npos) continue;
            line = line.substr(first);
            if (line.empty() || line[0] == '#') continue;

            std::istringstream ss(line);
            std::string token;
            ss >> token;

            if (token == "TITLE") continue;
            if (token == "LUT_3D_SIZE") {
                ss >> size;
                continue;
            }
            if (token == "LUT_1D_SIZE") {
                Log::error("LUT '{}' is a 1D LUT; only 3D LUTs are supported.", path_str);
                return false;
            }
            if (token == "DOMAIN_MIN") {
                float mn[3];
                ss >> mn[0] >> mn[1] >> mn[2];
                if (mn[0] != 0.0f || mn[1] != 0.0f || mn[2] != 0.0f) {
                    Log::error("LUT '{}' has non-zero DOMAIN_MIN ({}, {}, {}); HDR-domain LUTs are not supported.", path_str, mn[0], mn[1], mn[2]);
                    return false;
                }
                continue;
            }
            if (token == "DOMAIN_MAX") {
                float mx[3];
                ss >> mx[0] >> mx[1] >> mx[2];
                if (mx[0] != 1.0f || mx[1] != 1.0f || mx[2] != 1.0f) {
                    Log::error("LUT '{}' has non-unit DOMAIN_MAX ({}, {}, {}); HDR-domain LUTs are not supported.", path_str, mx[0], mx[1], mx[2]);
                    return false;
                }
                continue;
            }

            /* Data row. */
            glm::vec3 rgb;
            rgb.r = std::stof(token);
            ss >> rgb.g >> rgb.b;
            data.push_back(rgb);
        }
    } catch (const std::exception& e) {
        Log::error("LUT '{}' parsing failed: {}", path_str, e.what());
        return false;
    }

    if (size == 0) {
        Log::error("LUT '{}' is missing LUT_3D_SIZE header.", path_str);
        return false;
    }

    lut_size = (float)size;

    const size_t expected = size_t(size) * size_t(size) * size_t(size);
    if (data.size() != expected) {
        Log::error("LUT '{}' data row count mismatch: expected {}, got {}.", path_str, expected, data.size());
        return false;
    }

    /* Convert float32 RGB -> float16 RGBA. RGBA16Sfloat is 8 bytes per texel. */
    std::vector<uint16_t> halves(expected * 4);
    const uint16_t one_half = float_to_half(1.0f);
    for (size_t i = 0; i < expected; ++i) {
        halves[i * 4 + 0] = float_to_half(data[i].r);
        halves[i * 4 + 1] = float_to_half(data[i].g);
        halves[i * 4 + 2] = float_to_half(data[i].b);
        halves[i * 4 + 3] = one_half;
    }

    /* Get the VRAM bank */
    auto& bank = engine.renderer.vram_bank();

    /* Create the texture */
    const std::string texture_name = name + " Texture";
    texture = bank.create_texture(texture_name.c_str(), TextureUsage::Sampled | TextureUsage::TransferDst, TextureFormat::RGBA16Sfloat, { (uint32_t)size, (uint32_t)size, (uint32_t)size })
                  .expect("failed to initialise lut texture.");

    /* Initialise the image */
    std::string image_name = name + " Image";
    image = bank.create_image(image_name.c_str(), texture).expect("failed to initialize lut image.");

    bank.upload_texture(image, halves.data(), halves.size() * sizeof(uint16_t)).expect("failed to upload texture.");

    Log::info("LUT '{}' loaded sucessfuly.", path_str);

    return true;
}

void LUT::unload() {
    engine.renderer.destroy(texture);
    engine.renderer.destroy(image);
}

}  // namespace tmt
