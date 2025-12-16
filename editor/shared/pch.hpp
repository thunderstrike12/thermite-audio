#pragma once

#include <cstdint> /* uint32_t, int16_t, etc */
#include <cmath>   /* fminf, fmaxf, powf, etc */
#include <utility> /* std::swap */

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp> /* vector math */

#include "engine/tools/profiler.hpp" /* adds tracy profiling */
/* Large 32-bit floating point constant. */

#include "font/icon_lookups.hpp"

constexpr float BIG_F32 = 1e30f;

/* Number of voxels per unit in world-space. */
constexpr uint32_t VOXELS_PER_UNIT = 10u;
/* Number of world-space units per voxel. */
constexpr float UNITS_PER_VOXEL = 1.0f / VOXELS_PER_UNIT;
/* Half the size of a voxel in world-space units. */
constexpr float VOXEL_SIZE_HALF = 0.5f / VOXELS_PER_UNIT;
/* Size of a voxel in world-space units squared. */
constexpr float VOXEL_SIZE_SQR = UNITS_PER_VOXEL * UNITS_PER_VOXEL;
