#pragma once
#include "engine/core/io.hpp"

namespace tmt {

class Resource {
   public:
    virtual ~Resource() = default;

   protected:
    /* [Required] */
    virtual bool load() = 0;
    virtual void unload() = 0;
    /* [Optional] uses load by default */
    virtual bool reload() {
        unload();
        return load();
    }

    bool is_loaded() const { return loaded; }

   private:
    friend class Resources;
    /* Managed by Resources system */
    bool loaded = false;
};

/* Immutable, data *should* not be changed */
class FileResource : public Resource {
   protected:
    FileResource(IO::FileLocation file_location) : file_location(std::move(file_location)) {}

   public:
    virtual ~FileResource() = default;

    const IO::FileLocation file_location;
};

template <typename T>
concept ResourceType = std::derived_from<T, FileResource>;

/* Mutable, is allowed to be changed and can be shared between objects */
template <ResourceType T>
class RuntimeResource : public Resource {
   public:
    using ResourceType = T;
    virtual ~RuntimeResource() = default;

   protected:
    RuntimeResource(std::shared_ptr<T> resource) : file_resource(std::move(resource)) {}

    const std::shared_ptr<T> file_resource;
};
}  // namespace tmt