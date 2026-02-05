#include "goap_action_overrides.hpp"
#include "engine/core/io.hpp"

#include <string>
#include <fstream>

namespace tmt {

GoapActionEditorData& GoapActionOverrides::get(const std::string& id) { return data[id]; }

const GoapActionEditorData* GoapActionOverrides::find(const std::string& id) const {
    auto it = data.find(id);
    return (it != data.end()) ? &it->second : nullptr;
}

void GoapActionOverrides::load() {
    std::ifstream f("goap_actions.json");
    if (!f.is_open()) return;

    JsonReflect::json j;
    f >> j;
    Serializer::deserialize(j, *this);
}

void GoapActionOverrides::save() const {
    tmt::json j = Serializer::serialize(*this);
    std::ofstream f("goap_actions.json");
    f << j.dump(2);
}

}  // namespace tmt
