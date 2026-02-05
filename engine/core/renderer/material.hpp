#pragma once

namespace tmt {

/* Voxel material data, used in a palette. */
struct Material {
    float albedo_r = 0.0f;
    float albedo_g = 0.0f;
    float albedo_b = 0.0f;
    /* Padding */
    uint32_t : 32;
};

/* Material palette index. */
using MaterialIndex = uint8_t;

constexpr MaterialIndex AIR_INDEX = 0xFFu;

/* Voxel material palette with 256 entries. */
struct MaterialPalette {
    static constexpr size_t ENTRY_COUNT = (1u << (sizeof(MaterialIndex) * 8u));
    Material entries[ENTRY_COUNT] {};
};

}  // namespace tmt
