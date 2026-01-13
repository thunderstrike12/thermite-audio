#pragma once

/* Large 32-bit floating point constant. */
constexpr float BIG_F32 = 1e30f;

/* The square root of 2 */
constexpr float SQRT2 = 1.41421356237f;
/* The square root of 3 */
constexpr float SQRT3 = 1.73205080757f;

/* Number of voxels per unit in world-space. */
constexpr uint32_t VOXELS_PER_UNIT = 10u;
/* Number of world-space units per voxel. */
constexpr float UNITS_PER_VOXEL = 1.0f / VOXELS_PER_UNIT;
/* Half the size of a voxel in world-space units. */
constexpr float VOXEL_SIZE_HALF = 0.5f / VOXELS_PER_UNIT;
/* Size of a voxel in world-space units squared. */
constexpr float VOXEL_SIZE_SQR = UNITS_PER_VOXEL * UNITS_PER_VOXEL;