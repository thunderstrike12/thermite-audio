#pragma once
#include "engine/tools/fmt/helper.hpp"
#include <nlohmann/json.hpp>

FMT_LOGGING(nlohmann::json, "(json=\"{}\")", obj.dump());
FMT_LOGGING(nlohmann::ordered_json, "(json=\"{}\")", obj.dump());
