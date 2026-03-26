#pragma once

#include "editor/core/window.hpp"

namespace tmt {

class VoxelEditorLights : public IWindow<> {
   public:
    VoxelEditorLights() = default;
    ~VoxelEditorLights() override = default;

    // Inherited via IWindow
    void on_editor_update(const FrameData&) override;

    void on_inspect() override;

    [[nodiscard]] std::string get_title() const override { return "Editor Lights"; }
    [[nodiscard]] constexpr bool default_open() const override { return true; }
};

}  // namespace tmt
