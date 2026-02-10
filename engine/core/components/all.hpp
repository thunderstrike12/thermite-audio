#pragma once
#include "engine/tools/component_registry.hpp"

/* All components */
#include "engine/core/components/transform.hpp"
#include "engine/core/components/name.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/component_collection.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"
#include "engine/core/components/audio_listener.hpp"
#include "engine/core/components/audio_emitter.hpp"
#include "engine/core/components/prefab.hpp"
#include "engine/core/components/light.hpp"
#include "engine/core/components/emitter.hpp"
#include "engine/core/components/ui_component.hpp"
#include "engine/core/components/image_renderer.hpp"
#include "engine/tools/uuid.hpp"

namespace tmt {

// clang-format off
using SerializeComponents = ComponentRegistry<
	/* Order in which components are serialized */
	Prefab,
	Name, 
	Transform, 
	VoxelRenderer, 
	Camera,
	VoxelBody,
	ComponentCollection,
	NavMesh,
	AudioListener,
	AudioEmitter,
	Light,
	ParticleEmitter,
	UIComponent,
	ImageRenderer
>;
// clang-format on

// clang-format off
using InspectComponents = ComponentRegistry<
	/* Order in which components are displayed in the editor */
	Name, 
	Transform,
	VoxelRenderer, 
	Camera,
	VoxelBody,
	NavMesh,
	AudioListener,
	AudioEmitter,
	Light,
	ParticleEmitter,
	UIComponent,
	ImageRenderer
>;
// clang-format on

}  // namespace tmt