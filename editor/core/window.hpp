#pragma once
#include "editor/events/editor.hpp"
#include <string>

namespace tmt {
class IWindow : public IEditorEvents {
   public:
    virtual ~IWindow() = default;

    virtual void before_begin() {};
    virtual void display() = 0;
    virtual void end_display() {};

    constexpr virtual std::string get_title() const = 0;
    // should return ImGuiWindowFlags_* (combination)
    constexpr virtual int get_window_flags() const { return 0; }
    constexpr virtual bool is_closable() const { return true; }
};
}  // namespace tmt