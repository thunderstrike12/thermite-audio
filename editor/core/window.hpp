#pragma once
#include "editor/events/editor.hpp"
#include <string>

namespace tmt {
class IWindow : public IEditorEvents {
   public:
    virtual ~IWindow() = default;

    virtual void display() = 0;

    constexpr virtual std::string get_title() const = 0;
};
}  // namespace tmt