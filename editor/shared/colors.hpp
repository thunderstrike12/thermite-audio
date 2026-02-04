#pragma once

#include <glm/glm.hpp>
#include <imgui.h>

namespace tmt::colors {

/* Convert a GLM color to an ImGUI U32 color. */
inline ImU32 to_u32(const glm::vec3& rgb, const float alpha = 1.0f) { return ImGui::ColorConvertFloat4ToU32(ImVec4(rgb.x, rgb.y, rgb.z, alpha)); }

/* Red color, used for the X axis. */
constexpr glm::vec3 RED = glm::vec3(0.93725490f, 0.28235294f, 0.35686274f);
/* Green color, used for the Y axis. */
constexpr glm::vec3 GREEN = glm::vec3(0.52941176f, 0.81176470f, 0.21176470f);
/* Blue color, used for the Z axis. */
constexpr glm::vec3 BLUE = glm::vec3(0.27843137f, 0.56078431f, 0.94509803f);

/* Yellow/orange color, used for indicating something is selected. */
constexpr glm::vec3 SELECTED = glm::vec3(0.95686274f, 0.60392156f, 0.21960784f);

/* Dark color, used for gizmos. */
constexpr glm::vec3 DARK = glm::vec3(0.4f);
/* Light color, used for gizmos. */
constexpr glm::vec3 LIGHT = glm::vec3(0.8f);

}  // namespace tmt::colors
