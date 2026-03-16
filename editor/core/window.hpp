#pragma once
#include "editor/core/system.hpp"
#include <string>
namespace tmt {

/**
 * @brief Interface for editor systems that have a visible inspector window.
 *
 * Inherits from IEditorSystem for lifecycle callbacks and adds window-specific
 * functionality like display(), get_title(), and window flags.
 *
 * If your system doesn't need an inspector window, use IEditorSystem directly instead.
 */
class IWindow : public IEditorSystem {
   public:
    virtual ~IWindow() = default;

    /* [Required] */
    virtual void on_inspect() = 0;

    /* [Optional] */
    virtual void before_begin() {};
    virtual void end_display() {};

    /* [Required] */
    constexpr virtual std::string get_title() const = 0;

    /* [Optional] */
    // should return ImGuiWindowFlags_* (combination)
    constexpr virtual int get_window_flags() const { return 0; }
    constexpr virtual bool is_closable() const { return true; }
    constexpr virtual bool default_open() const { return false; }
};

}  // namespace tmt