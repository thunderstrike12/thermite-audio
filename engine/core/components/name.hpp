#pragma once
#include <string>

#include "engine/tools/fmt/helper.hpp"

namespace tmt {
struct Name {
    std::string name {"Unnamed Entity"};
};
}  // namespace tmt

FMT_LOGGING(tmt::Name, "name: \"{}\"", obj.name);