#include "steering_params_overrides.hpp"

#include "engine/core/io.hpp"

#include "core/logger.hpp"

#include <string>
#include <fstream>

namespace tmt {

void SteeringOverrides::load() {
    const std::string file_data = IO::read_text_file({ IO::Location::PROJECT, "ai/steering_params.json" });
    if (file_data.empty()) {
        tmt::Log::warn(tmt::Log::Scope::ENGINE, "No GOAP actions found at steering_params.json, starting with empty registry.");
        return;
    }

    tmt::json j {};
    try {
        j = tmt::json::parse(file_data);
    } catch (const std::exception& e) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to parse steering_params.json: {}", e.what());
        return;
    }

    Serializer::deserialize(j, *this);
}


void SteeringOverrides::save() const {
    tmt::json j = Serializer::serialize(*this);
    bool success = IO::write_text_file({ IO::Location::PROJECT, "ai/steering_params.json" }, j.dump(4));
    if (!success) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to save steering parameters");
    }
}

}  // namespace tmt
