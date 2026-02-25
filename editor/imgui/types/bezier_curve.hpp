#pragma once

#include <ImReflect.hpp>

#include "engine/tools/types/bezier_curve.hpp"

void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::BezierCurve& curve, ImSettings&, ImResponse& response);