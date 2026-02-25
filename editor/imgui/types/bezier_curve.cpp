#include "bezier_curve.hpp"

#include "extern/imgui-bezier-editor/bezier_editor.hpp"

void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::BezierCurve& curve, ImSettings&, ImResponse& response) {
    ImGui::Bezier(label, curve.values.data());  // draw
}