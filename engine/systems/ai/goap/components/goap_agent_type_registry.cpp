#include "goap_agent_type_registry.hpp"

#include "engine/core/io.hpp"
#include "engine/core/logger.hpp"

namespace tmt {

void GoapAgentTypeRegistry::load() {
    const std::string file_data = IO::read_text_file({ IO::Location::PROJECT, "ai/goap_agents.json" });
    if (file_data.empty()) {
        tmt::Log::warn(tmt::Log::Scope::ENGINE, "No GOAP agent types found at goap_agents.json, starting with empty registry.");
        return;
    }

    tmt::json j {};
    try {
        j = tmt::json::parse(file_data);
    } catch (const std::exception& e) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to parse goap_agents.json: {}", e.what());
        return;
    }

    Serializer::deserialize(j, *this);
}

void GoapAgentTypeRegistry::save() const {
    tmt::json j = Serializer::serialize(*this);
    IO::write_text_file({ IO::Location::PROJECT, "ai/goap_agents.json" }, j.dump(4));
}

}  // namespace tmt
