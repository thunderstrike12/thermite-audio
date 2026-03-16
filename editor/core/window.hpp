#pragma once
#include "editor/core/system.hpp"
#include <string>
namespace tmt {

class IWindowBase {
   public:
    virtual ~IWindowBase() = default;

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

template <typename Derived = void>
class IWindow : public IEditorSystem<Derived>, public IWindowBase {};

}  // namespace tmt