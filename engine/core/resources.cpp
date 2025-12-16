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
            tmt::Log::info(tmt::Log::Scope::ENGINE, "Unloaded unused file resource {}", file_location);
            resource_type_locations[resources.at(file_location).type_hash].erase(file_location);  // Get the type hash
            resources.erase(file_location);
            i--;
        }
    }
}

size_t Resources::resource_count() const { return resources.size(); }

void Resources::ResourceCollection::push_back(const std::shared_ptr<Resource>& resource) { runtime_resources.push_back(resource); }

void Resources::ResourceCollection::clean_up() {
    std::erase_if(runtime_resources, [](const std::weak_ptr<Resource>& res) { return res.expired(); });
}

}  // namespace tmt