#pragma once
#include <memory>
#include "resource.hpp"
#include "logger.hpp"

namespace tmt {

class Resources {
   public:
    /* file resources loading */
    template <ResourceType T, typename... Args>
        requires std::constructible_from<T, const IO::FileLocation&, Args...>
    std::shared_ptr<T> load_resource(const IO::FileLocation& file_location, Args&&... args) {
        // duplicate checking
        if (resources.contains(file_location)) {
            auto& collection = resources.at(file_location);
            auto casted = std::dynamic_pointer_cast<T>(collection.file_resource);
            if (!casted) {
                Log::error(Log::Scope::ENGINE, "Type mismatch: resource at '{}' already loaded as different type", file_location);
            }
            return casted;
        }

        std::shared_ptr<T> resource = std::make_shared<T>(file_location, std::forward<Args>(args)...);

        // load() success checking
        if (!resource->load()) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to load resource!");
            resource->unload();
            resource->loaded = false;
            return nullptr;
        }
        resource->loaded = true;

        resources.emplace(file_location, resource);

        return std::dynamic_pointer_cast<T>(resource);
    }

    /* Runtime resources loading that takes in a file resource */
    template <typename T>
        requires std::derived_from<T, RuntimeResource<typename T::ResourceType>>
    std::shared_ptr<T> copy_resource(const std::shared_ptr<typename T::ResourceType>& file_resource) {
        if (file_resource == nullptr) {
            Log::error(Log::Scope::ENGINE, "Cannot copy nullptr file resource");
            return nullptr;
        }
        if (resources.contains(file_resource->file_location) == false) {
            Log::error(Log::Scope::ENGINE, "File resource at '{}' not managed by Resources! Should not happen.", file_resource->file_location);
            return nullptr;
        }

        std::shared_ptr<T> resource = std::make_shared<T>(file_resource);

        if (!resource->load()) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to load runtime resource!");
            resource->unload();
            resource->loaded = false;
            return nullptr;
        }
        resource->loaded = true;

        auto& collection = resources.at(file_resource->file_location);
        collection.push_back(resource);
        return resource;
    }

    /* Runtime resources loading that takes in a file resource args */
    template <typename T, typename... Args>
        requires std::derived_from<T, RuntimeResource<typename T::ResourceType>> && std::constructible_from<typename T::ResourceType, const IO::FileLocation&, Args...>
    std::shared_ptr<T> copy_resource(const IO::FileLocation& file_location, Args&&... args) {
        auto file_resource = load_resource<typename T::ResourceType>(file_location, std::forward<Args>(args)...);
        if (file_resource == nullptr) {
            return nullptr;
        }
        return copy_resource<T>(file_resource);
    }

    void unload_unused();

    size_t resource_count() const;

   private:
    class ResourceCollection {
       public:
        ResourceCollection(std::shared_ptr<FileResource> file_resource) : file_resource(std::move(file_resource)) {}

        /* Remove expired runtime resources */
        void clean_up();

        void push_back(const std::shared_ptr<Resource>& resource);
        auto begin() {
            clean_up();
            return runtime_resources.begin();
        }
        auto end() { return runtime_resources.end(); }

        const std::shared_ptr<FileResource> file_resource;

       private:
        std::vector<std::weak_ptr<Resource>> runtime_resources;
    };

    /* File location -> file resource & runtime resources */
    std::unordered_map<IO::FileLocation, ResourceCollection, IO::FileLocationHash> resources;
};
}  // namespace tmt