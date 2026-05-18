#pragma once

#include <ImReflect.hpp>

namespace tmt {
	class AudioEmitter;
}

void tag_invoke(ImReflect::ImInput_t, const char*, tmt::AudioEmitter& value, ImSettings& settings, ImResponse& response);