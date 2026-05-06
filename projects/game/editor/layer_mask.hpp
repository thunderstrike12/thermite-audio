#pragma once
#include <ImReflect.hpp>
namespace game {

struct LayerMask;

}

void tag_invoke(ImReflect::ImInput_t, const char* name, game::LayerMask& mask, ImSettings& settings, ImResponse& response);