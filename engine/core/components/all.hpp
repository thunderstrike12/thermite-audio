#pragma once
#include <type_traits>
#include "engine/tools/component_registry.hpp"

/* All components */
#include "engine/core/components/transform.hpp"
#include "engine/core/components/name.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/component_collection.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/systems/physics/components/destructable.hpp"
#include "engine/systems/animation/components/constraints.hpp"
#include "engine/systems/animation/components/bone_hierarchy_renderer.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"
#include "engine/core/components/audio_listener.hpp"
#include "engine/core/components/audio_emitter.hpp"
#include "engine/core/components/prefab.hpp"
#include "engine/core/components/light.hpp"
#include "engine/core/components/emitter.hpp"
#include "engine/core/components/ui_component.hpp"
#include "engine/core/components/image_renderer.hpp"
#include "engine/core/components/text_renderer.hpp"
#include "engine/core/components/button.hpp"
#include "engine/core/components/environment.hpp"
#include "engine/systems/animation/rig_model.hpp"
#include "engine/tools/uuid.hpp"
#include "engine/systems/ai/goap/components/goap_agent_type_ref.hpp"
#include "engine/systems/ai/goap/components/goap_agent_type_registry.hpp"
#include "engine/systems/ai/goap/components/goap_goal.hpp"
#include "engine/systems/ai/goap/components/goap_action.hpp"
#include "engine/systems/ai/goap/components/world_state.hpp"
#include "engine/core/components/rig_controller.hpp"

namespace tmt {

// clang-format off
using SerializeComponents = ComponentRegistry<
	/* Order in which components are serialized */
	Prefab,
	Name,
	Transform, 
	DisableFlag,
	VoxelRenderer, 
	Environment,
	Camera,
	VoxelBody,
	Destructible,
	ComponentCollection,
	NavMesh,
	AnimConstraints::DampedTransformConstraint,
	AnimConstraints::TwoBoneIKConstraint,
	AnimConstraints::EffectorWalkCycle,
	NoBone,
	BendHint,
	Effector,
	BoneHierarchyRenderer,
	AudioListener,
	AudioEmitter,
	Light,
	ParticleEmitter,
	UIComponent,
	ImageRenderer,
	TextRenderer,
	Button,
	BoneComp,
	RigModel,
	GoapAgentType,
	GoapAgentTypeRef,
	WorldState,
	RigController
>;
// clang-format on

// clang-format off
using InspectComponents = ComponentRegistry<
	/* Order in which components are displayed in the editor */
	//Prefab,
	Name, 
	Transform,
	VoxelRenderer, 
	Environment,
	Camera,
	VoxelBody,
	Destructible,
	NavMesh,
	AnimConstraints::DampedTransformConstraint,
	AnimConstraints::TwoBoneIKConstraint,
	AnimConstraints::EffectorWalkCycle,
	NoBone,
	BendHint,
	Effector,
	BoneHierarchyRenderer,
	AudioListener,
	AudioEmitter,
	Light,
	ParticleEmitter,
	UIComponent,
	ImageRenderer,
	TextRenderer,
	Button,
	RigModel,
	GoapAgentType,
	GoapAgentTypeRef,
	WorldState,
	RigController
>;
// clang-format on

}  // namespace tmt

// template <typename T>
// requires(tmt::SerializeComponents::contains<T>())
// struct JsonReflect::Detail::delta_serialize<T> : std::true_type {};