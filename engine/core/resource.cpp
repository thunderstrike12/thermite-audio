#include "resource.hpp"
#include "resource.hpp"

namespace tmt {
Resource::Resource(const IO::FileLocation& _file_location) : file_location(_file_location) {}
ExampleResource::ExampleResource(const IO::FileLocation& location, float arg) : Resource({}) { important_float = arg; }
bool ExampleResource::load() { return true; }
void ExampleResource::unload() {}
bool ExampleResource::reload() { return true; }
}  // namespace tmt
