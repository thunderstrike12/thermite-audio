#pragma once
#include <memory>
#include "resource.hpp"
#include "logger.hpp"

namespace tmt {

template <typename T>
concept ResourceType = std::derived_from<T, Resource>;

class Resources {
   public:
    // loads or gets the resource of type T with file location file_location
    template <ResourceType T, typename... Args>
        requires std::constructible_from<T, const IO::FileLocation&, Args...>
    std::shared_ptr<T> load_resource(const IO::FileLocation& file_location, Args&&... args) {
        // duplicate checking
        if (resources.contains(file_location)) {
            // already loaded
            return std::dynamic_pointer_cast<T>(resources[file_location]);
        }

        std::shared_ptr<T> resource = std::make_shared<T>(file_location, std::forward<Args>(args)...);

        // load() success checking
        if (!resource->load()) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to load resource!");
            resource->unload();
            return nullptr;
        }

        resources.emplace(file_location, resource);

        return std::dynamic_pointer_cast<T>(resource);
    }

   private:
    std::unordered_map<IO::FileLocation, std::shared_ptr<Resource>, IO::FileLocationHash> resources;
};
}  // namespace tmt