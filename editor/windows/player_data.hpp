#pragma once
#include "editor/core/window.hpp"

namespace tmt {

class PlayerDataWindow : public IWindow {
   public:
    std::string get_title() const override { return "Player Data"; };
    void on_inspect() override;
};

}  // namespace tmt