#pragma once
#include <memory>
#include <unordered_set>

#include "resource.hpp"
#include "logger.hpp"

namespace tmt {

class Resources {
   private:
    class ResourceCollection {
       public:
        ResourceCollection(std::shared_ptr<FileResource> file_resource, const size_t type_hash) : file_resource(std::move(file_resource)), type_hash(type_hash) {}

        /* Remove expired runtime resources */
        void clean_up();

        void push_back(const std::shared_ptr<Resource>& resource);
        auto begin() {
            clean_up();
            return runtime_resources.begin();
        }
        auto end() { return runtime_resources.end(); }

        const std::shared_ptr<FileResource> file_resource;
        const size_t type_hash;

       private:
        std::vector<std::weak_ptr<Resource>> runtime_resources;
    };

   public:
    /* file resources loading */
    template <ResourceType T, typename... Args>
    requires std::constructible_from<T, const IO::FileLocation&, Args...>
    ResourceRef<T> load_resource(const IO::FileLocation& file_location, Args&&... args) {
        // duplicate checking
        if (resources.contains(file_location)) {
            auto& collection = resources.at(file_location);
            const TimeStamp last_modified_time = IO::get_file_last_modified_time(file_location);

            if (collection.file_resource->last_modified_time != last_modified_time) {
                /* File has been modified since last load */
                Log::info(Log::Scope::ENGINE, "[Resources] File resource at '{}' has been modified, reloading...", file_location);
                reload_collection(collection);
            }

            auto casted = std::dynamic_pointer_cast<T>(collection.file_resource);
            if (!casted) {
                Log::error(Log::Scope::ENGINE, "[Resources] Type mismatch: resource at '{}' already loaded as different type", file_location);
            }
            ResourceRef<T> ref(file_location, casted);
            return ref;
        }

        std::shared_ptr<T> resource = std::make_shared<T>(file_location, std::forward<Args>(args)...);
        resource->last_modified_time = IO::get_file_last_modified_time(file_location);

        // load() success checking
        if (!resource->load()) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "[Resources] Failed to load resource!");
            resource->unload();
            resource->loaded = false;
            ResourceRef<T> ref(file_location);
            return ref;
        }
        resource->loaded = true;

        const size_t type_hash = typeid(T).hash_code();
        resources.emplace(std::piecewise_construct, std::forward_as_tuple(file_location), std::forward_as_tuple(resource, type_hash));
        resource_type_locations[type_hash].emplace(file_location);

        ResourceRef<T> ref(file_location, std::dynamic_pointer_cast<T>(resource));
        return ref;
    }

    /* Runtime resources loading that takes in a file resource */
    template <typename T, typename... Args>
    requires requires { typename T::is_runtime_resource; } && std::constructible_from<T, const std::shared_ptr<typename T::ResourceType>&, Args...>
    ResourceRef<T> copy_resource(const ResourceRef<typename T::ResourceType>& file_resource, Args&&... args) {
        if (file_resource == nullptr) {
            Log::error(Log::Scope::ENGINE, "[Resources] Cannot copy nullptr file resource");
            ResourceRef<T> ref;
            return ref;
        }
        if (resources.contains(file_resource->file_location) == false) {
            Log::error(Log::Scope::ENGINE, "[Resources] File resource at '{}' not managed by Resources! Should not happen.", file_resource->file_location);
            ResourceRef<T> ref(file_resource->file_location);
            return ref;
        }

        std::shared_ptr<T> resource = std::make_shared<T>(file_resource.resource, std::forward<Args>(args)...);

        if (!resource->load()) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "[Resources] Failed to load runtime resource!");
            resource->unload();
            resource->loaded = false;
            ResourceRef<T> ref(file_resource->file_location);
            return ref;
        }
        resource->loaded = true;

        auto& collection = resources.at(file_resource->file_location);
        collection.push_back(resource);
        ResourceRef<T> ref(file_resource->file_location, std::dynamic_pointer_cast<T>(resource));
        return ref;
    }

    /* Runtime resources loading that takes in a file location (loads file resource first, then creates runtime resource) */
    template <typename T, typename... Args>
    requires requires { typename T::is_runtime_resource; } && std::constructible_from<typename T::ResourceType, const IO::FileLocation&, Args...>
    ResourceRef<T> copy_resource(const IO::FileLocation& file_location, Args&&... args) {
        auto file_resource = load_resource<typename T::ResourceType>(file_location, std::forward<Args>(args)...);
        if (file_resource == nullptr) {
            ResourceRef<T> ref(file_location);
            return ref;
        }
        return copy_resource<T>(file_resource);
    }

    void unload_unused();

    void force_unload_all();

    size_t resource_count() const;

    bool reload_resource(const std::shared_ptr<Resource>& resource) const;
    bool reload_resource(const std::shared_ptr<FileResource>& resource) const;
    void reload_collection(ResourceCollection& collection) const;

    /* Get a set containing the file locations of all the resources with the provided type */
    template <ResourceType T>
    const std::unordered_set<IO::FileLocation, IO::FileLocationHash>& get_resource_locations() {
        return resource_type_locations[typeid(T).hash_code()];
    }

   private:
    std::unordered_map<size_t, std::unordered_set<IO::FileLocation, IO::FileLocationHash>> resource_type_locations;

    /* File location -> file resource & runtime resources */
    std::unordered_map<IO::FileLocation, ResourceCollection, IO::FileLocationHash> resources;
};

}  // namespace tmt