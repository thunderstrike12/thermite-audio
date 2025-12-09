#pragma once

namespace tmt {

/* Voxel material data, used in a palette. */
struct Material {
    float albedo_r = 0.0f;
    float albedo_g = 0.0f;
    float albedo_b = 0.0f;
};

/* Material palette index. */
using MaterialIndex = uint8_t;

/* Voxel material palette with 255 entries. */
struct MaterialPalette {
    static constexpr size_t ENTRY_COUNT = (1u << (sizeof(MaterialIndex) * 8u)) - 1u;
    Material entries[ENTRY_COUNT] {};

    /* Subscript operator */
    Material& operator[](size_t i) { return entries[i + 1u]; }
    const Material& operator[](size_t i) const { return entries[i + 1u]; }
};

}  // namespace tmt
