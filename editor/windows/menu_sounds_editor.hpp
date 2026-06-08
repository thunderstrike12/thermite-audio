#pragma once

#include "editor/core/window.hpp"

namespace tmt {

class MenuSoundsEditor : public tmt::IWindow<> {
   public:
    std::string get_title() const override { return "Menu Sounds"; }

    void on_editor_start() override;
    void on_editor_end() override;
    void on_inspect() override;
};

}  // namespace tmt
