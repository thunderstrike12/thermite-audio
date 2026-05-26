#pragma once

#include "editor/core/window.hpp"

namespace tmt {

class NodePaletteDiff;

class MaterialEditor : public IWindow<> {
   public:
    constexpr std::string get_title() const override { return ICON_MS_EDIT " Material Editor"; }
    constexpr bool default_open() const override { return true; }

    void on_inspect() override;

    void on_editor_start() override {}
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override {}

    bool edit_all_palettes { true };

   private:
    // Reused diff for material properties.
    NodePaletteDiff* diff { nullptr };

    template <typename Type>
    void handle_material_change(Entity selected_entity, const std::vector<MaterialIndex>& material_indices, Type Material::* member, Type after_value, bool activated = false);
};

}  // namespace tmt