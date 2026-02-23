#pragma once
#include <string>
#include "engine/core/reflection.hpp"

namespace tmt {

struct GoapAgentTypeRef {
    std::string type_id;
};

}  // namespace tmt

TMT_COMPONENT(tmt::GoapAgentTypeRef, "GoapAgentTypeRef", (type_id));
