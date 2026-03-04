#pragma once
#include "glm/fwd.hpp"
#include "engine/core/reflection.hpp"
namespace tmt
{
struct BoneHierarchyRenderer
{
	glm::vec4 color;
	float line_width;
};
struct NoBone {};
struct BendHint 
{
	float radius;
};
struct Effector {
	float radius;
};
}  // namespace tmt
TMT_COMPONENT(tmt::BoneHierarchyRenderer, "BoneHierarchyRenderer", (color, line_width));
TMT_COMPONENT(tmt::BendHint, "Bend Hint", (radius));
TMT_COMPONENT(tmt::Effector, "Effector", (radius));

TMT_COMPONENT_NAME(tmt::NoBone, "No Bone");
TMT_COMPONENT_SERIALIZE_EMPTY(tmt::NoBone);
TMT_COMPONENT_INSPECT_EMPTY(tmt::NoBone);