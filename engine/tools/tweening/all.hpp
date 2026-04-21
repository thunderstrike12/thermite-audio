#pragma once
#include "engine/tools/component_registry.hpp"

#include "transform.hpp"

namespace tmt {

// clang-format off
template <typename SubTweenType>
using SubTweenMixinsInspect = ComponentRegistry<
	transform_tweening::WorldOrLocalMixin<SubTweenType>, 
	transform_tweening::OffsetMixin<SubTweenType, typename SubTweenType::value_type>
>;
// clang-format on

}  // namespace tmt