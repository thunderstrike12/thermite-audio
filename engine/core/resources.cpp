#include "resources.hpp"

namespace tmt {

void Resources::unload_unused() {
    for (int i = 0; i < resources.size(); i++) {
        auto& [file_location, collection] = *std::next(resources.begin(), i);
        collection.clean_up();

        const int ref_count = collection.file_resource.use_count();
        if (ref_count <= 1) {
            collection.file_resource->unload();
            collection.file_resource->loaded = false;
            tmt::Log::debug(tmt::Log::Scope::ENGINE, "[Resources] Unloaded unused file resource {}", file_location);
            resource_type_locations[resources.at(file_location).type_hash].erase(file_location);  // Get the type hash
            resources.erase(file_location);
            i--;
        }
    }
}

void Resources::force_unload_all() {
    for (auto& [file_location, collection] : resources) {
        const int ref_count = collection.file_resource.use_count();

        if (ref_count > 1) {
            tmt::Log::debug(tmt::Log::Scope::ENGINE, "[Resources] Force unloading file resource {} with {} active references!", file_location, ref_count - 1);
        } else {
            tmt::Log::debug(tmt::Log::Scope::ENGINE, "[Resources] Force unloaded file resource {}", file_location);
        }

        collection.file_resource->unload();
        collection.file_resource->loaded = false;
    }
    resources.clear();
    resource_type_locations.clear();
}

size_t Resources::resource_count() const {
    return resources.size();
}

void Resources::reload_collection(ResourceCollection& collection) const {
    const bool success = reload_resource(collection.file_resource);
    if (!success) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "[Resources] Failed to reload resource collection");
        return;
    }

    for (auto& runtime_resource : collection) {
        if (auto res = runtime_resource.lock()) {
            if (!reload_resource(res)) {
                tmt::Log::error(tmt::Log::Scope::ENGINE, "[Resources] Failed to reload runtime resource in collection");
            }
        }
    }
}

bool Resources::reload_resource(const std::shared_ptr<Resource>& resource) const {
    const bool success = resource->reload();
    if (!success) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "[Resources] Failed to reload resource");
        resource->fallback(FallbackReason::RELOAD_FAILED);
        return false;
    }
    resource->loaded = true;
    return true;
}

bool Resources::reload_resource(const std::shared_ptr<FileResource>& resource) const {
    const auto last_modified_time = IO::get_file_last_modified_time(resource->file_location);
    // Check if the file to reload from exists.
    if (last_modified_time == TimeStamp::min()) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "[Resources] Failed to reload resource, couldn't find file: {}", resource->file_location);
        return false;
    }

    const bool success = resource->reload();
    if (!success) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "[Resources] Failed to reload resource: {}", resource->file_location);
        resource->fallback(FallbackReason::RELOAD_FAILED);
        return false;
    }
    resource->last_modified_time = last_modified_time;
    resource->loaded = true;
    return true;
}

void Resources::ResourceCollection::push_back(const std::shared_ptr<Resource>& resource) {
    runtime_resources.push_back(resource);
}

void Resources::ResourceCollection::clean_up() {
    std::erase_if(runtime_resources, [](const std::weak_ptr<Resource>& res) { return res.expired(); });
}

}  // namespace tmt