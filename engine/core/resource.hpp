#pragma once
#include "engine/core/io.hpp"

namespace tmt {
class Resource {
   protected:
    Resource(const IO::FileLocation& file_location);

   public:
    virtual ~Resource() = default;

    virtual bool load() = 0;
    virtual void unload() = 0;
    virtual bool reload() = 0;

    const IO::FileLocation file_location;
};

class ExampleResource : public Resource {
   public:
    ExampleResource(const IO::FileLocation& location, float example_arg);

    // Inherited via Resource
    bool load() override;
    void unload() override;
    bool reload() override;

    float important_float;

   private:
};
}  // namespace tmt