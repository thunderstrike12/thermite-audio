#pragma once

#include "engine/core/resource.hpp"

#include <vector>

namespace tmt {

struct VoxelSceneNode;
class VoxelScene;

bool decode_svh(const std::vector<char>& data, VoxelSceneNode& hierarchy);
std::vector<char> encode_svh(const VoxelScene& scene);

}  // namespace tmt