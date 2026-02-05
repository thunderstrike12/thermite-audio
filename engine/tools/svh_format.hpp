#pragma once

#include "engine/core/resource.hpp"

#include <vector>

namespace tmt {

struct VoxelSceneNode;

std::vector<VoxelSceneNode> decode_svh(const std::vector<char>& data);
std::vector<char> encode_svh(const std::span<VoxelSceneNode>& root_nodes);

}  // namespace tmt