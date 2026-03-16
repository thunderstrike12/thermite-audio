#pragma once

#include "engine/tools/serializer.hpp"
#include "engine/events/debug.hpp"

#include "editor/core/window.hpp"

namespace tmt {

class Rendering : public IEditorSystem, public OnDrawLines {
   public:
    void on_draw_lines() const override;
    constexpr std::string get_name() const override { return "Rendering"; };
};

}  // namespace tmt
