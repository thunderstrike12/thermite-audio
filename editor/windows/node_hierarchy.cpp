#include "node_hierarchy.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/tools/file_dialog.hpp"
#include "engine/tools/svh_format.hpp"
#include "engine/shared/colorspace.hpp"
#include "engine/shared/const.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#define OGT_VOXEL_MESHIFY_IMPLEMENTATION
#include <ogt_voxel_meshify.h>

#include <fstream>

namespace tmt {

namespace {

std::vector<uint8_t> create_uniform_voxels(const std::unique_ptr<Svt64>& tree, const glm::uvec3& size) {
    const uint32_t voxel_count = size.x * size.y * size.z;
    std::vector<uint8_t> voxels(voxel_count, 0);

    for (uint32_t x = 0; x < size.x; x++) {
        for (uint32_t y = 0; y < size.y; y++) {
            for (uint32_t z = 0; z < size.z; z++) {
                const Material* material = tree->get_voxel(x, y, z);
                if (material == nullptr) continue;

                const uint8_t palette_index = static_cast<uint8_t>(material - tree->palette.entries);
                voxels[x + y * size.x + z * size.x * size.y] = palette_index + 1;  // Add one to account for 0 being air usually.
            }
        }
    }

    return voxels;
}

void drag_node(const Entity entity, const std::string& name) {
    if (!ImGui::BeginDragDropSource()) return;

    ImGui::SetDragDropPayload("VoxelNode", &entity, sizeof(entity));
    ImGui::Text("%s", name.c_str());

    ImGui::EndDragDropSource();
}

struct NodeCreationData {
    Entity parent {entt::null};
    std::string name {"New Node"};

    bool has_voxel_grid {false};
    glm::uvec3 voxel_size {1, 1, 1};
};
std::unique_ptr<NodeCreationData> node_creation_info;

struct VoxelEditUUID {
    UUID uuid {NULL_UUID};
};

Entity recurse_build_scene(const VoxelSceneNode& node, bool assign_new_uuids, const Entity parent_entity = entt::null, const glm::mat4& parent_matrix = glm::identity<glm::mat4>()) {
    const Entity entity = engine.ecs.create_entity(node.name);

    Transform& transform = engine.ecs.get_component<Transform>(entity);
    transform.set_world_matrix(parent_matrix * node.transform);
    transform.set_parent(parent_entity);

    VoxelEditUUID& uuid_component = engine.ecs.add_component<VoxelEditUUID>(entity);
    uuid_component.uuid = (assign_new_uuids ? UUIDGenerator::generate() : node.uuid);

    if (node.tree) {
        VoxelRenderer& renderer = engine.ecs.add_component<VoxelRenderer>(entity);

        // Create a fake voxel resource managed by the voxel editor.
        const ResourceRef volume {{}, std::make_shared<VoxelVolume>(node)};
        volume->uuid = uuid_component.uuid;
        renderer.resource = volume;
    }

    for (const VoxelSceneNode& child : node.children) {
        recurse_build_scene(child, assign_new_uuids, entity, transform.get_world_matrix());
    }

    return entity;
}

void recurse_parse_scene(Entity entity, const Transform& transform, VoxelSceneNode& node) {
    node.name = engine.ecs.get_component<Name>(entity).name;

    const glm::mat4 translation = glm::translate(glm::identity<glm::mat4>(), transform.get_local_position());
    const glm::mat4 rotation = glm::toMat4(transform.get_local_rotation());
    const glm::mat4 scale = glm::scale(glm::identity<glm::mat4>(), transform.get_local_scale());
    node.transform = translation * rotation * scale;

    node.uuid = engine.ecs.get_component<VoxelEditUUID>(entity).uuid;

    const VoxelRenderer* renderer = engine.ecs.try_get_component<VoxelRenderer>(entity);
    if (renderer != nullptr) {
        node.uuid = renderer->resource->uuid;
        node.size = renderer->resource->size;
        node.tree = std::make_unique<Svt64>(*renderer->resource->blas);
    }

    for (const Entity child : transform.get_children()) {
        const Transform& child_transform = engine.ecs.get_component<Transform>(child);
        recurse_parse_scene(child, child_transform, node.children.emplace_back());
    }
}

std::atomic_bool model_load_atomic {true};

}  // namespace

void NodeHierarchy::new_svh() {
    clear_hierarchy();
    loaded_location = {};
}

void NodeHierarchy::open_svh() {
    open_file_dialog(
        [this](const IO::FileLocation& location) {
            model_load_atomic.store(false);

            clear_hierarchy();
            loaded_location = location;

            const ResourceRef<VoxelScene> resource = engine.resources.load_resource<VoxelScene>(location);
            build_scene(resource->root_nodes);

            model_load_atomic.store(true);
            model_load_atomic.notify_one();
        },
        {{"Thermite Voxel File", "svh"}}
    );
}

void NodeHierarchy::save_svh() {
    if (loaded_location.relative_path.empty()) {
        save_svh_as();
        return;
    }

    const std::vector<char> scene_data = encode_voxel_scene();
    IO::write_file(loaded_location, scene_data.data(), scene_data.size());
}

void NodeHierarchy::save_svh_as() {
    save_file_dialog(
        [this](const IO::FileLocation& location) {
            loaded_location = location;

            const std::vector<char> scene_data = encode_voxel_scene();
            IO::write_file(location, scene_data.data(), scene_data.size());
        },
        {{"Thermite Voxel File", "svh"}}
    );
}

void NodeHierarchy::import_file(const std::string& file_description, const std::string& file_extension) {
    open_file_dialog(
        [this](const IO::FileLocation& location) {
            model_load_atomic.store(false);

            const ResourceRef<VoxelScene> resource = engine.resources.load_resource<VoxelScene>(location);
            build_scene(resource->root_nodes, true);

            model_load_atomic.store(true);
            model_load_atomic.notify_one();
        },
        {{file_description.c_str(), file_extension.c_str()}}
    );
}

void NodeHierarchy::export_file(const std::string& file_description, const std::string& file_extension) const {
    save_file_dialog(
        [this](const IO::FileLocation& location) {
            const entt::basic_group renderer_group = engine.ecs.get_registry().group<VoxelRenderer>(entt::get<Transform>);
            if (renderer_group.empty()) return;

            const std::filesystem::path obj_path = location.get_relative_path();
            const std::filesystem::path mtl_path = location.get_relative_path().replace_extension(".mtl");

            std::ofstream obj_file {obj_path, std::ios::trunc};

            if (obj_file.is_open()) obj_file << std::format("mtllib {}", mtl_path.filename().generic_string()) << '\n';
            std::ofstream mtl_file {mtl_path, std::ios::trunc};

            uint32_t indices_offset = 0;
            for (const auto&& [entity, renderer, transform] : renderer_group.each()) {
                const std::string& name = engine.ecs.get_component<Name>(entity).name;
                const std::string& uuid_string = engine.ecs.get_component<VoxelEditUUID>(entity).uuid.str();

                const ResourceRef<VoxelVolume>& resource = renderer.resource;
                const std::unique_ptr<Svt64>& tree = resource->blas;
                const glm::uvec3& size = resource->size;

                // Loop over all the voxels in the Svt64 tree to create a uniform voxel grid.
                const std::vector<uint8_t> uniform_voxels = create_uniform_voxels(tree, size);

                constexpr ogt_mesh_rgba palette[256] {};  // We don't really need to initialize the array, we only care about the indices, so we just need to allocate it for the ogt function.
                constexpr ogt_voxel_meshify_context context {};
                // Create a mesh from the uniform voxel grid.
                const ogt_mesh* mesh = ogt_mesh_from_paletted_voxels_polygon(&context, uniform_voxels.data(), size.x, size.y, size.z, palette);

                if (obj_file.is_open()) {
                    // Start new names object in the wavefront object file.
                    obj_file << std::format("o {}", name) << '\n';

                    // Write all visuals to the file.
                    for (uint32_t i = 0; i < mesh->vertex_count; i++) {
                        const glm::vec3& local_coord = glm::make_vec3(&mesh->vertices[i].pos.x) - (glm::vec3 {size} * 0.5f);
                        const glm::vec3& coord = transform.get_world_matrix() * glm::vec4 {local_coord * UNITS_PER_VOXEL, 1.0f};

                        // Invert the X-axis to correctly load it in external programs.
                        obj_file << std::format("v {} {} {}", -coord.x, coord.y, coord.z) << '\n';
                    }

                    // Write all the polygons using the indices (+1 because .obj indices start at 1).
                    for (uint32_t i = 0; i < mesh->index_count; i += 3) {
                        // Use the material with the correct color.
                        obj_file << std::format("usemtl {}_{}", uuid_string, mesh->vertices[mesh->indices[i]].palette_index - 1) << '\n';

                        obj_file << std::format("f {} {} {}", indices_offset + mesh->indices[i + 0] + 1, indices_offset + mesh->indices[i + 1] + 1, indices_offset + mesh->indices[i + 2] + 1)
                                 << '\n';
                    }
                    indices_offset += mesh->vertex_count;
                }

                if (mtl_file.is_open()) {
                    // Write the colors to the mtl file.
                    for (uint32_t i = 0; i < 256; i++) {
                        // Start a new material.
                        mtl_file << std::format("newmtl {}_{}", uuid_string, i) << '\n';

                        // Write the color then the alpha, which is always 1.0.
                        const Material& material = tree->palette.entries[i];
                        mtl_file << std::format("Kd {} {} {}", cs::linearize(material.albedo_r), cs::linearize(material.albedo_g), cs::linearize(material.albedo_b)) << '\n';
                        mtl_file << "d 1.0" << '\n';
                    }
                }
            }

            obj_file.close();
            mtl_file.close();
        },
        {{file_description.c_str(), file_extension.c_str()}}
    );
}

void NodeHierarchy::build_scene(const std::span<VoxelSceneNode>& root_nodes, bool assign_new_uuids) {
    for (VoxelSceneNode& root_node : root_nodes) {
        const Entity root_entity = recurse_build_scene(root_node, assign_new_uuids);

        root_entities.emplace(root_entity);
    }
}

std::vector<char> NodeHierarchy::encode_voxel_scene() {
    const auto& entities = engine.ecs.get_registry().storage<Transform>();
    if (entities.empty()) return {};  // Return empty scene encoding.

    std::vector<VoxelSceneNode> root_nodes;
    // For each root entity, recursively build a VoxelSceneNode of it and its children.
    for (const Entity entity : root_entities) {
        Transform& transform = engine.ecs.get_component<Transform>(entity);

        VoxelSceneNode& root_node = root_nodes.emplace_back();
        recurse_parse_scene(entity, transform, root_node);
    }

    clear_hierarchy();

    // Encode those VoxelSceneNodes into the .svh format.
    return encode_svh(root_nodes);
}

void NodeHierarchy::recurse_display_node(const Entity entity, const Name& name, Transform& transform) {
    ImGui::PushID(static_cast<int>(entity));

    const std::set<Entity>& children = transform.get_children();

    constexpr ImGuiTreeNodeFlags default_flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DrawLinesToNodes |
                                                 ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    ImGuiTreeNodeFlags flags = default_flags;
    flags |= (children.empty() ? ImGuiTreeNodeFlags_Leaf : 0);
    flags |= (entity == selected_entity ? ImGuiTreeNodeFlags_Selected : 0);

    const bool tree_open = ImGui::TreeNodeEx(name.name.c_str(), flags);
    node_context_menu(entity);
    drag_node(entity, name.name);
    drop_node(entity);

    const bool popup_clicked_open = (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup));
    // Both left mouse button selects and opening the popup also selects, this makes it clear what node entity you have opened the popup for.
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || popup_clicked_open) selected_entity = entity;
    if (tree_open) {
        for (const Entity child : children) {
            assert(engine.ecs.valid(child));
            auto&& [child_name, child_transform] = engine.ecs.get_component<Name, Transform>(child);
            recurse_display_node(child, child_name, child_transform);
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}

void NodeHierarchy::clear_hierarchy() {
    selected_entity = entt::null;
    root_entities.clear();
    engine.ecs.clear();
}

void NodeHierarchy::drop_hierarchy() {
    const ImRect window_rect {ImGui::GetWindowContentRegionMin(), ImGui::GetWindowContentRegionMax()};
    if (!ImGui::BeginDragDropTargetCustom(window_rect, ImGui::GetID("NodeEmptyDropArea"))) return;

    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("VoxelNode");
    if (payload != nullptr) {
        const Entity dropped_entity = *static_cast<Entity*>(payload->Data);

        engine.ecs.get_component<Transform>(dropped_entity).clear_parent();
        root_entities.emplace(dropped_entity);
    }

    ImGui::EndDragDropTarget();
}

void NodeHierarchy::drop_node(const Entity entity) {
    if (!ImGui::BeginDragDropTarget()) return;

    const ImGuiPayload* payload = ImGui::GetDragDropPayload();
    if (payload != nullptr && payload->IsDataType("VoxelNode")) {
        const Entity dropped_entity = *static_cast<Entity*>(payload->Data);

        if (entity != dropped_entity && ImGui::AcceptDragDropPayload("VoxelNode")) {
            // We have to get the entity's transform again here, otherwise entt can't find the entity that has the transform, I have no idea why.
            Transform& transform = engine.ecs.get_component<Transform>(entity);
            transform.add_child(dropped_entity);

            if (root_entities.contains(dropped_entity)) root_entities.erase(dropped_entity);
        }
    }

    ImGui::EndDragDropTarget();
}

void NodeHierarchy::popup_create_node() {
    // Store the popup id so other functions can use it to open the popup.
    if (popup_id == 0) popup_id = ImGui::GetCurrentWindow()->GetID("Create New Node");

    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;
    if (!ImGui::BeginPopupModal("Create New Node", nullptr, flags)) return;

    ImGui::InputText("Node name", &node_creation_info->name);

    bool& has_voxel_grid = node_creation_info->has_voxel_grid;
    ImGui::Checkbox("Has voxel grid", &has_voxel_grid);

    if (has_voxel_grid) {
        constexpr glm::uvec2 min_max {1, 1024};
        ImGui::DragScalarN("Size", ImGuiDataType_U32, &node_creation_info->voxel_size.x, 3, 0.25f, &min_max.x, &min_max.y);
    }

    if (ImGui::Button("Create")) {
        const Entity new_node_entity = engine.ecs.create_entity(node_creation_info->name);

        if (node_creation_info->parent != entt::null)
            engine.ecs.get_component<Transform>(new_node_entity).set_parent(node_creation_info->parent);
        else
            root_entities.emplace(new_node_entity);

        UUID uuid;
        if (has_voxel_grid) {
            // Create a fake voxel resource managed by the voxel editor.
            const ResourceRef volume {{}, std::make_shared<VoxelVolume>(node_creation_info->voxel_size)};
            uuid = volume->uuid;
            engine.ecs.add_component<VoxelRenderer>(new_node_entity).resource = volume;
        }

        if (uuid == NULL_UUID) uuid = UUIDGenerator::generate();
        engine.ecs.add_component<VoxelEditUUID>(new_node_entity).uuid = uuid;

        ImGui::CloseCurrentPopup();
        node_creation_info.reset();
    }
    if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
        node_creation_info.reset();
    }

    ImGui::EndPopup();
}

void NodeHierarchy::node_context_menu(const Entity node_entity) const {
    constexpr ImGuiPopupFlags flags = ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_MouseButtonRight;
    if (!ImGui::BeginPopupContextItem(nullptr, flags)) return;

    if (ImGui::MenuItem(ICON_MS_ADD "Add node")) {
        node_creation_info = std::make_unique<NodeCreationData>();
        node_creation_info->parent = node_entity;
        ImGui::OpenPopupEx(popup_id);
    }
    if (ImGui::MenuItem(ICON_MS_REMOVE " Delete node")) engine.ecs.destroy_entity(node_entity);

    ImGui::EndPopup();
}

void NodeHierarchy::display() {
    model_load_atomic.wait(false);

    popup_create_node();

    std::vector<Entity> test;
    for (const Entity entity : root_entities) {
        auto&& [transform, name] = engine.ecs.get_component<Transform, Name>(entity);

        recurse_display_node(entity, name, transform);
    }
    // Erase all root entities that have been marked for delete.
    std::erase_if(root_entities, [](const Entity& entity) { return !engine.ecs.valid(entity) || engine.ecs.has_component<Delete>(entity); });

    constexpr ImGuiPopupFlags flags = ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_MouseButtonRight;
    if (ImGui::BeginPopupContextWindow(nullptr, flags)) {
        if (ImGui::MenuItem("Add node")) {
            node_creation_info = std::make_unique<NodeCreationData>();
            ImGui::OpenPopupEx(popup_id);
        }
        ImGui::EndPopup();
    }

    drop_hierarchy();
}

}  // namespace tmt