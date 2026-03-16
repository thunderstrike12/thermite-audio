#include "goap_action_overrides.hpp"
#include "engine/core/io.hpp"

#include "core/logger.hpp"

#include <string>
#include <fstream>

namespace tmt {

GoapActionEditorData& GoapActionOverrides::get(const std::string& id) {
    return data[id];
}

const GoapActionEditorData* GoapActionOverrides::find(const std::string& id) const {
    auto it = data.find(id);
    return (it != data.end()) ? &it->second : nullptr;
}

void GoapActionOverrides::load() {
    /*std::ifstream f("goap_actions.json");
    if (!f.is_open()) return;

    JsonReflect::json j;
    f >> j;
    Serializer::deserialize(j, *this);*/

    const std::string file_data = IO::read_text_file({ IO::Location::PROJECT, "ai/goap_actions.json" });
    if (file_data.empty()) {
        tmt::Log::warn(tmt::Log::Scope::ENGINE, "No GOAP actions found at goap_actions.json, starting with empty registry.");
        return;
    }

    tmt::json j {};
    try {
        j = tmt::json::parse(file_data);
    } catch (const std::exception& e) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to parse goap_actions.json: {}", e.what());
        return;
    }

    Serializer::deserialize(j, *this);
}

void GoapActionOverrides::save() const {
    /*tmt::json j = Serializer::serialize(*this);
    std::ofstream f("goap_actions.json");
    f << j.dump(2);*/
    tmt::json j = Serializer::serialize(*this);
    IO::write_text_file({ IO::Location::PROJECT, "ai/goap_actions.json" }, j.dump(4));
}

}  // namespace tmt
