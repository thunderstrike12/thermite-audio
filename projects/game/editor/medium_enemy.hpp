#pragma once
#include <ImReflect.hpp>

namespace game {

class MediumEnemy;

}

void tag_invoke(ImReflect::ImInput_t, const char* name, game::MediumEnemy& value, ImSettings& settings, ImResponse& response);