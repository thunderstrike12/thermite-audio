#pragma once

#include <string>
#include <any>

namespace tmt {

class IEditorMode {
   public:
    IEditorMode() = default;
    virtual ~IEditorMode() = default;

    constexpr virtual std::string get_name() = 0;

    virtual void display_main_menu() = 0;

    virtual void on_switch_to(const std::any& meta_data = {}) = 0;
    virtual void on_switch_away() = 0;
};

}  // namespace tmt