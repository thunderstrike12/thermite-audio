#pragma once
/* Large 32-bit floating point constant. */
constexpr float BIG_F32 = 1e30f;

constexpr int VOXELS_PER_UNIT = 10;
constexpr float UNITS_PER_VOXEL = 1.0f / VOXELS_PER_UNIT;
constexpr float VOXEL_SIZE_HALF = 0.5f / VOXELS_PER_UNIT;
constexpr float VOXEL_SIZE_SQR = UNITS_PER_VOXEL * UNITS_PER_VOXEL;