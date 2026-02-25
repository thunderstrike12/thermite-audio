#pragma once
#include <set>

#include "engine/core/io.hpp"
#include "engine/core/reflection.hpp"

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

    const IO::FileLocation file_location {};

   private:
    friend class Resources;
    TimeStamp last_modified_time = {};
};

template <typename T>
concept ResourceType = std::derived_from<T, FileResource>;

/* Mutable, is allowed to be changed and can be shared between objects */
template <ResourceType T>
class RuntimeResource : public Resource {
   public:
    using ResourceType = T;
    using is_runtime_resource = std::true_type;

    virtual ~RuntimeResource() = default;

    inline static const std::set<std::string_view>& SUPPORTED_FILE_EXTENSIONS { T::SUPPORTED_FILE_EXTENSIONS };  // NOLINT(readability-identifier-naming)

    const std::shared_ptr<T> file_resource;

   protected:
    RuntimeResource(std::shared_ptr<T> resource) : file_resource(std::move(resource)) {}
};

template <typename T>
class ResourceRef {
   public:
    using ResourceType = T;

    ResourceRef() = default;
    ResourceRef(IO::FileLocation file_location) : file_location(std::move(file_location)), resource(nullptr) {}
    ResourceRef(IO::FileLocation file_location, std::shared_ptr<T> resource) : file_location(std::move(file_location)), resource(std::move(resource)) {}

    IO::FileLocation file_location;
    std::shared_ptr<T> resource = nullptr;

    T* operator->() const { return resource.get(); }

    explicit operator bool() const { return resource != nullptr; }

    bool operator==(const ResourceRef<T>& other) const { return file_location == other.file_location && resource == other.resource; }
    bool operator==(std::nullptr_t) const { return resource == nullptr; }
};

}  // namespace tmt

TMT_OBJECT(tmt::FileResource, (file_location));