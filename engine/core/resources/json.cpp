#include "json.hpp"
#include "engine/core/logger.hpp"

namespace tmt {

bool Json::load() {
    if (IO::file_exists(file_location) == false) {
        Log::error(Log::Scope::ENGINE, "[Json] File does not exisits: {}", file_location);
        return false;
    }

    const std::string file_content = IO::read_text_file(file_location);
    if (file_content.empty()) {
        Log::error(Log::Scope::ENGINE, "[Json] File is empty: {}", file_location);
        return false;
    }

    nlohmann::ordered_json temp_json;
    try {
        temp_json = nlohmann::json::parse(file_content, nullptr, true);
    } catch (json::parse_error& ex) {
        Log::error(Log::Scope::ENGINE, "[Json] Failed to parse JSON: {}", ex.what());
        return false;
    }

    parsed_json = std::move(temp_json);
    return true;
}
void Json::unload() {
    parsed_json.clear();
}

}  // namespace tmt