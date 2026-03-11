#include "bezier_curve.hpp"

// Suppress warnings from external imgui-bezier-editor library
#pragma warning(push)
#pragma warning(disable : 4005)  // macro redefinition
#pragma warning(disable : 4100)  // unreferenced formal parameter
#pragma warning(disable : 4127)  // conditional expression is constant
#pragma warning(disable : 4189)  // local variable is initialized but not referenced
#pragma warning(disable : 4244)  // conversion from 'float' to 'int', possible loss of data
#pragma warning(disable : 5054)  // operator between enumerations of different types deprecated
#pragma warning(disable : 5055)  // operator between enumerations and floating-point types deprecated
#include "extern/imgui-bezier-editor/bezier_editor.hpp"
#pragma warning(pop)

void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::BezierCurve& curve, ImSettings& /*settings*/, ImResponse& /*response*/) {
    ImGui::Bezier(label, curve.values.data());  // draw
}