#pragma once
#include <nlohmann/json.hpp>
#include "engine/core/resource.hpp"

namespace tmt {
class Json : public FileResource {
   public:
    Json(IO::FileLocation file_location) : FileResource(std::move(file_location)) {};

    bool load() override;
    void unload() override;

    const nlohmann::ordered_json& get_parsed_json() const { return parsed_json; }

   private:
    nlohmann::ordered_json parsed_json;
};
}  // namespace tmt