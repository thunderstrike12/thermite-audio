#pragma once
#include "editor/core/window.hpp"

namespace tmt {

class EditorSettingsWindow : public IWindow<> {
   public:
    virtual void on_inspect() override;

    constexpr virtual std::string get_title() const override { return ICON_MS_SETTINGS " Editor Settings"; }
};

}  // namespace tmt