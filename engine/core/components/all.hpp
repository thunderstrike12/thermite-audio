#pragma once
#include "engine/tools/component_registry.hpp"

/* All components */
#include "engine/core/components/transform.hpp"
#include "engine/core/components/name.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/components/camera.hpp"

namespace tmt {

// clang-format off
using AllComponents = ComponentRegistry<
	/* Components */
	Name, 
	Transform, 
	VoxelRenderer, 
	Camera
>;
// clang-format on
}  // namespace tmt