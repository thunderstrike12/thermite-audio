#pragma once
#include <nlohmann/json.hpp>
#include "engine/core/resource.hpp"

namespace tmt {

class Json : public FileResource {
   public:
    Json(IO::FileLocation file_location) : FileResource(std::move(file_location)) {};

    bool load() override;
    void unload() override;

    inline static const std::set<std::string_view> SUPPORTED_FILE_EXTENSIONS { ".json" };

    const nlohmann::ordered_json& get_parsed_json() const { return parsed_json; }
    nlohmann::ordered_json& get_parsed_json() { return parsed_json; }

   private:
    nlohmann::ordered_json parsed_json;
};

}  // namespace tmt