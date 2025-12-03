#pragma once
#include "engine/tools/fmt/helper.hpp"

#include <filesystem>

FMT_LOGGING(std::filesystem::path, "{}", obj.string());