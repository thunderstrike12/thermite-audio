#pragma once

#include <graphite/resources/handle.hh>

#include "engine/shared/rgb10.hpp"

namespace tmt {

/* Voxel material data, used in a palette. */
struct Material {
    enum class Type : uint8_t { NONE };

    Rgb10 albedo {};
    uint16_t ior { 0x3C00 };      /* f16 encoding, 1.0f by default. */
    uint16_t emission { 0x0000 }; /* f16 encoding. 0.0f by default. */
    uint8_t roughness { 0 };
    uint8_t metallic { 0 };
    uint8_t transmission { 0 };
    Type type { Type::NONE };
};

/* Material palette index. */
using MaterialIndex = uint8_t;

constexpr MaterialIndex AIR_INDEX = 0xFFu;

/* Voxel material palette with 256 entries. */
struct MaterialPalette {
    static constexpr size_t ENTRY_COUNT = (1u << (sizeof(MaterialIndex) * 8u));
    Material entries[ENTRY_COUNT] {};

    MaterialIndex material_to_index(const Material* material) const { return static_cast<MaterialIndex>(material - entries); }
};

/* Generate/precompute directional albedo look-up texture. */
void generate_e_lut(Texture& out_texture, const uint32_t resolution = 32u);

}  // namespace tmt
