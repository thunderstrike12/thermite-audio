#pragma once

#include <fstream>
#include <unordered_map>
#include <vector>

#include "goap_action_editor_data.hpp"
#include "goap_action.hpp"

#include "engine/core/reflection.hpp"

namespace tmt {

/**
 * Class GoapActionOverrides
 * Container that stores editor overrides for all GOAP actions.
 *
 * Provides runtime access to overridden costs, preconditions, and effects.
 */
class GoapActionOverrides {
   public:
    // Map of action ID to override data
    std::unordered_map<std::string, GoapActionEditorData> data;

    // Access override data for the given action ID.
    GoapActionEditorData& get(const std::string& id);

    // Find existing override data for the given action ID. Returns nullptr if not found.
    const GoapActionEditorData* find(const std::string& id) const;

    void load();
    void save() const;
};

}  // namespace tmt

TMT_OBJECT(tmt::GoapActionOverrides, (data));
