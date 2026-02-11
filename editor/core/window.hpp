#pragma once
#include "editor/events/editor.hpp"
#include <string>
namespace tmt {

class IWindow : public IEditorEvents {
   public:
    virtual ~IWindow() = default;

    /* [Required] */
    virtual void display() = 0;

    /* [Optional] */
    virtual void before_begin() {};
    virtual void end_display() {};

    /* IEditorEvents made optional */
    virtual void on_editor_start() {};
    virtual void on_editor_update(const tmt::FrameData& time) { (void)time; };
    virtual void on_editor_end() {};

    /* [Required] */
    constexpr virtual std::string get_title() const = 0;

    /* [Optional] */
    // should return ImGuiWindowFlags_* (combination)
    constexpr virtual int get_window_flags() const { return 0; }
    constexpr virtual bool is_closable() const { return true; }
    constexpr virtual bool default_open() const { return false; }
};

}  // namespace tmt