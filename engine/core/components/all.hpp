#pragma once
#include "engine/tools/component_registry.hpp"

/* All components */
#include "engine/core/components/transform.hpp"
#include "engine/core/components/name.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/components/camera.hpp"

#include "engine/systems/physics/components/voxel_body.hpp"

namespace tmt {

// clang-format off
using AllComponents = ComponentRegistry<
	/* Components */
	Name, 
	Transform, 
	VoxelRenderer, 
	Camera,
	VoxelBody
>;
// clang-format on
}  // namespace tmt