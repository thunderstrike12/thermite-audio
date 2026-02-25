#pragma once

/* 10-bit SDR color format. (4 bytes) */
struct Rgb10 {
    /* Packed RGB data. */
    uint32_t packed = 0u;

    static constexpr float SCALE = (float)(1u << 10) - 1.0f;
    static constexpr float RCP_SCALE = 1.0f / SCALE;

    Rgb10() = default;
    Rgb10(const uint32_t packed) : packed { packed } {}
    Rgb10(const glm::vec3& rgb) {
        const glm::uvec3 rgb_u32 = glm::clamp(rgb, 0.0f, 1.0f) * SCALE;
        packed = (rgb_u32.r & 0x3FFu) | ((rgb_u32.g & 0x3FFu) << 10) | ((rgb_u32.b & 0x3FFu) << 20);
    }

    /* Unpack RGB10 into vec3 color. */
    glm::vec3 unpack() const { return glm::vec3((float)(packed & 0x3FFu) * RCP_SCALE, (float)((packed >> 10) & 0x3FFu) * RCP_SCALE, (float)((packed >> 20) & 0x3FFu) * RCP_SCALE); }
};

/* Make sure the RGB10 format is 4 bytes in size. */
static_assert(sizeof(Rgb10) == sizeof(uint32_t));