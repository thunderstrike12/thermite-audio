#pragma once

#include "editor/core/window.hpp"
#include "engine/core/resource.hpp"
#include "engine/core/entity.hpp"

namespace tmt {

struct VoxelSceneNode;
struct Name;
struct Transform;

class VoxelVolume;

class NodeHierarchy : public IWindow {
   public:
    struct NodeUUID {
        UUID uuid { NULL_UUID };
    };

    constexpr std::string get_title() const override { return ICON_MS_FLOWCHART " Node hierarchy"; }
    constexpr bool default_open() const override { return true; }

    [[nodiscard]] bool is_entity_selected(Entity entity);
    [[nodiscard]] const std::vector<Entity>& get_selected_entities() const { return selected_entities; }
    [[nodiscard]] Entity get_first_selected_entity() const;
    void add_selected_entity(Entity entity);
    void set_selected_entity(Entity entity);
    void remove_selected_entity(Entity entity);
    void clear_selected_entities();

    void new_svh();
    void open_svh();
    void open_svh(const IO::FileLocation& location);
    void save_svh();
    void save_svh_as();
    void import_file(const std::string& file_description, const std::string& file_extension);
    void export_file(const std::string& file_description, const std::string& file_extension) const;

    void build_scene(const std::span<VoxelSceneNode>& root_nodes, bool assign_new_uuids = false, const std::map<UUID, Entity>& previous_entity_mapping = {});

    [[nodiscard]] std::vector<char> encode_voxel_scene() const;
    void clear_root_entities() { root_entities.clear(); }

   private:
    friend class VoxelNodeDiff;

    static void recalculate_all_physics();
    void recurse_display_node(Entity entity, const Name& name, Transform& transform, std::vector<Entity>& all_entities);
    void clear_hierarchy();

    void drop_hierarchy();
    void drop_node(Entity entity, Transform& transform);

    void popup_create_node();
    void popup_resize_node();
    void node_context_menu(Entity node_entity);

    void on_inspect() override;

    void on_editor_start() override {}
    void on_editor_update(const FrameData&) override;
    void on_editor_end() override {}

    // Popup ids, will be set when the popups are first created, allows us to open them from anywhere in imgui (hacky workaround).
    uint32_t creation_popup_id { 0 };
    uint32_t resize_popup_id { 0 };

    IO::FileLocation loaded_location {};

    std::vector<Entity> selected_entities;
    std::vector<Entity> root_entities;
};

}  // namespace tmt