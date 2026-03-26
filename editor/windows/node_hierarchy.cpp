#include "node_hierarchy.hpp"

#include "editor.hpp"
#include "editor/core/systems/undo_redo/component_diff.hpp"
#include "editor/core/systems/undo_redo/voxel_edit_diff.hpp"
#include "editor/core/systems/undo_redo/undo_redo_manager.hpp"
#include "editor/core/systems/undo_redo/entity_diff.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/shared/const.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/resources.hpp"
#include "engine/tools/svh_format.hpp"
#include "engine/shared/colorspace.hpp"
#include "engine/tools/file_dialog.hpp"
#include "engine/tools/serializer/all.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/systems/physics/physics_system.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#define OGT_VOXEL_MESHIFY_IMPLEMENTATION
#include <ogt_voxel_meshify.h>

#include <fstream>

namespace {

void apply_requests(ImGuiMultiSelectIO* io, std::vector<tmt::Entity>& selection, const std::vector<tmt::Entity>& entities) {
    for (const auto& request : io->Requests) {
        switch (request.Type) {
            case ImGuiSelectionRequestType_None:
                break;

            case ImGuiSelectionRequestType_SetAll:
                selection.clear();
                if (request.Selected) selection = entities;
                break;

            case ImGuiSelectionRequestType_SetRange: {
                if (request.Selected) {
                    for (ImGuiSelectionUserData i = request.RangeFirstItem; i <= request.RangeLastItem; i++) {
                        selection.push_back(entities.at(i));
                    }
                } else {
                    for (ImGuiSelectionUserData i = request.RangeFirstItem; i <= request.RangeLastItem; i++) {
                        selection.erase(std::ranges::find(selection, *(entities.begin() + i)));
                    }
                }
            } break;
        }
    }
}

void drag3_uint32(const char* label, uint32_t values[3], const float speed = 0.1f, const uint32_t& min = 0u, const uint32_t& max = 0u) {
    ImGui::DragScalarN(label, ImGuiDataType_U32, values, 3, speed, &min, &max, nullptr, ImGuiSliderFlags_ClampOnInput);
}

void slider3_uint32(const char* label, uint32_t values[3], const glm::uvec3& min, const glm::uvec3& max) {
    ImGui::PushID(label);
    ImGui::BeginGroup();

    ImGui::Text("%s", label);
    ImGui::SameLine();

    const float full_width = ImGui::CalcItemWidth();

    ImGui::SetNextItemWidth(full_width / 3.0f);
    ImGui::SliderScalarN("##X", ImGuiDataType_U32, &values[0], 1, &min[0], &max[0], nullptr, ImGuiSliderFlags_ClampOnInput);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(full_width / 3.0f);
    ImGui::SliderScalarN("##Y", ImGuiDataType_U32, &values[1], 1, &min[1], &max[1], nullptr, ImGuiSliderFlags_ClampOnInput);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(full_width / 3.0f);
    ImGui::SliderScalarN("##Z", ImGuiDataType_U32, &values[2], 1, &min[2], &max[2], nullptr, ImGuiSliderFlags_ClampOnInput);

    ImGui::EndGroup();
    ImGui::PopID();
}

std::vector<uint8_t> create_uniform_voxels(const std::unique_ptr<tmt::Svt64>& tree, const glm::uvec3& size) {
    const uint32_t voxel_count = size.x * size.y * size.z;
    std::vector<uint8_t> voxels(voxel_count, 0);

    for (uint32_t x = 0; x < size.x; x++) {
        for (uint32_t y = 0; y < size.y; y++) {
            for (uint32_t z = 0; z < size.z; z++) {
                const tmt::Material* material = tree->get_voxel(x, y, z);
                if (material == nullptr) continue;

                const uint8_t palette_index = static_cast<uint8_t>(material - tree->palette.entries);
                voxels[x + y * size.x + z * size.x * size.y] = palette_index + 1;  // Add one to account for 0 being air usually.
            }
        }
    }

    return voxels;
}

void drag_node(const tmt::Entity entity, const std::string& name) {
    if (!ImGui::BeginDragDropSource()) return;

    ImGui::SetDragDropPayload("VoxelNode", &entity, sizeof(entity));
    ImGui::Text("%s", name.c_str());

    ImGui::EndDragDropSource();
}

struct NodeCreationData {
    tmt::Entity parent { entt::null };
    std::string name { "New Node" };

    bool has_voxel_grid { false };
    glm::uvec3 voxel_size { 1, 1, 1 };
};
std::unique_ptr<NodeCreationData> node_creation_info;

struct NodeResizeData {
    tmt::Entity entity;
    glm::uvec3 size;
    glm::uvec3 offset;
};
std::unique_ptr<NodeResizeData> node_resize_info;

tmt::Entity recurse_build_scene(
    const tmt::VoxelSceneNode& node, bool assign_new_uuids, const std::map<tmt::UUID, tmt::Entity>& previous_entity_mapping, const tmt::Entity parent_entity = entt::null,
    const glm::mat4& parent_matrix = glm::identity<glm::mat4>()
) {
    const tmt::UUID uuid = (assign_new_uuids ? tmt::UUIDGenerator::generate() : node.uuid);

    tmt::Entity entity = previous_entity_mapping.contains(uuid) ? previous_entity_mapping.at(uuid) : entt::null;
    entity = tmt::engine.ecs.create_entity(node.name, entity);

    tmt::Transform& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    transform.set_parent(parent_entity);
    transform.set_world_matrix(parent_matrix * node.transform);

    tmt::NodeHierarchy::NodeUUID& uuid_component = tmt::engine.ecs.add_component<tmt::NodeHierarchy::NodeUUID>(entity);
    uuid_component.uuid = uuid;

    if (node.tree) {
        tmt::VoxelRenderer& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);

        // Create a fake voxel resource managed by the voxel editor.
        const tmt::ResourceRef volume { {}, std::make_shared<tmt::VoxelVolume>(node) };
        volume->uuid = uuid_component.uuid;
        renderer.resource = volume;
    }

    for (const tmt::VoxelSceneNode& child : node.children) {
        recurse_build_scene(child, assign_new_uuids, previous_entity_mapping, entity, transform.get_world_matrix());
    }

    return entity;
}

void recurse_parse_scene(tmt::Entity entity, const tmt::Transform& transform, tmt::VoxelSceneNode& node) {
    node.name = tmt::engine.ecs.get_component<tmt::Name>(entity).name;

    const glm::mat4 translation = glm::translate(glm::identity<glm::mat4>(), transform.get_local_position());
    const glm::mat4 rotation = glm::toMat4(transform.get_local_rotation());
    const glm::mat4 scale = glm::scale(glm::identity<glm::mat4>(), transform.get_local_scale());
    node.transform = translation * rotation * scale;

    node.uuid = tmt::engine.ecs.get_component<tmt::NodeHierarchy::NodeUUID>(entity).uuid;

    const tmt::VoxelRenderer* renderer = tmt::engine.ecs.try_get_component<tmt::VoxelRenderer>(entity);
    if (renderer != nullptr) {
        node.uuid = renderer->resource->uuid;
        node.size = renderer->resource->size;
        node.tree = std::make_unique<tmt::Svt64>(*renderer->resource->blas);
    }

    for (const tmt::Entity child : transform.get_children()) {
        const tmt::Transform& child_transform = tmt::engine.ecs.get_component<tmt::Transform>(child);
        recurse_parse_scene(child, child_transform, node.children.emplace_back());
    }
}

tmt::Entity recurse_duplicate_node(const tmt::Entity source_entity, const tmt::Entity parent = entt::null) {
    const auto& [source_name, source_transform] = tmt::engine.ecs.get_component<tmt::Name, tmt::Transform>(source_entity);

    const tmt::Entity entity = tmt::engine.ecs.create_entity(source_name.name);
    auto&& [name, transform] = tmt::engine.ecs.get_component<tmt::Name, tmt::Transform>(entity);

    transform.set_world_matrix(source_transform.get_world_matrix());
    transform.set_parent(parent);

    tmt::NodeHierarchy::NodeUUID& uuid_component = tmt::engine.ecs.add_component<tmt::NodeHierarchy::NodeUUID>(entity);
    uuid_component.uuid = tmt::UUIDGenerator::generate();

    const tmt::VoxelRenderer* source_renderer = tmt::engine.ecs.try_get_component<tmt::VoxelRenderer>(source_entity);
    if (source_renderer != nullptr) {
        tmt::VoxelRenderer& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);

        // Create a fake voxel resource managed by the voxel editor.
        const tmt::ResourceRef volume { {}, std::make_shared<tmt::VoxelVolume>(source_renderer->resource, uuid_component.uuid) };
        renderer.resource = volume;
    }

    for (const tmt::Entity& child : source_transform.get_children()) {
        recurse_duplicate_node(child, entity);
    }

    return entity;
}

std::atomic_bool model_load_atomic { true };

}  // namespace

namespace tmt {

bool NodeHierarchy::is_entity_selected(Entity entity) {
    // Linear search because we use an std vector, but we need to keep our own order so there is no good alternative.
    return std::ranges::find(selected_entities, entity) != selected_entities.end();
}

Entity NodeHierarchy::get_first_selected_entity() const {
    if (selected_entities.empty()) return entt::null;

    return *selected_entities.begin();
}

void NodeHierarchy::add_selected_entity(const Entity entity) {
    if (entity == entt::null || is_entity_selected(entity)) return;

    NodeVectorDiff diff { selected_entities };
    diff.before();
    selected_entities.push_back(entity);
    diff.after();
    IUndoRedo::send_to_manager(std::move(diff), "Add Node Selection");
}

void NodeHierarchy::set_selected_entity(const Entity entity) {
    if (selected_entities.size() == 1 && selected_entities.front() == entity) return;

    NodeVectorDiff diff { selected_entities };
    diff.before();

    selected_entities.clear();
    selected_entities.push_back(entity);

    diff.after();
    IUndoRedo::send_to_manager(std::move(diff), "Set Node Selection");
}

void NodeHierarchy::remove_selected_entity(const Entity entity) {
    const auto iterator = std::ranges::find(selected_entities, entity);

    if (iterator == selected_entities.end()) return;

    NodeVectorDiff diff { selected_entities };
    diff.before();
    selected_entities.erase(iterator);
    diff.after();
    IUndoRedo::send_to_manager(std::move(diff), "Remove Node Selection");
}

void NodeHierarchy::clear_selected_entities() {
    if (selected_entities.empty()) return;

    NodeVectorDiff diff { selected_entities };
    diff.before();
    selected_entities.clear();
    diff.after();
    IUndoRedo::send_to_manager(std::move(diff), "Clear Node Selection");
}

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
        { { "Thermite Voxel File", "svh" } }
    );
}

void NodeHierarchy::open_svh(const IO::FileLocation& location) {
    clear_hierarchy();
    loaded_location = location;

    const ResourceRef<VoxelScene> resource = engine.resources.load_resource<VoxelScene>(location);
    build_scene(resource->root_nodes);
}

void NodeHierarchy::save_svh() {
    if (loaded_location.relative_path.empty()) {
        save_svh_as();
        return;
    }

    recalculate_all_physics();

    const std::vector<char> scene_data = encode_voxel_scene();
    IO::write_file(loaded_location, scene_data.data(), scene_data.size());
}

void NodeHierarchy::save_svh_as() {
    save_file_dialog(
        [this](const IO::FileLocation& location) {
            loaded_location = location;
            if (loaded_location.relative_path.extension() != ".svh") loaded_location.relative_path += ".svh";  // Make sure the saved file has the correct extension.

            recalculate_all_physics();

            const std::vector<char> scene_data = encode_voxel_scene();
            IO::write_file(loaded_location, scene_data.data(), scene_data.size());
        },
        { { "Thermite Voxel File", "svh" } }
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
        { { file_description.c_str(), file_extension.c_str() } }
    );
}

void NodeHierarchy::export_file(const std::string& file_description, const std::string& file_extension) const {
    save_file_dialog(
        [this](const IO::FileLocation& location) {
            const entt::basic_group renderer_group = engine.ecs.group<VoxelRenderer>(entt::get<Transform>);
            if (renderer_group.empty()) return;

            // Make sure all the paths have the correct extension.
            IO::FileLocation obj_location = location;
            if (obj_location.relative_path.extension() != ".obj") obj_location.relative_path += ".obj";
            const std::filesystem::path obj_path = obj_location.get_relative_path();
            const std::filesystem::path mtl_path = obj_location.get_relative_path().replace_extension(".mtl");

            std::ofstream obj_file { obj_path, std::ios::trunc };

            if (obj_file.is_open()) obj_file << std::format("mtllib {}", mtl_path.filename().generic_string()) << '\n';
            std::ofstream mtl_file { mtl_path, std::ios::trunc };

            uint32_t indices_offset = 0;
            for (const auto&& [entity, renderer, transform] : renderer_group.each()) {
                const std::string& name = engine.ecs.get_component<Name>(entity).name;
                const std::string& uuid_string = engine.ecs.get_component<NodeUUID>(entity).uuid.str();

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
                    obj_file << std::format("g {}", name) << '\n';

                    // Write all visuals to the file.
                    for (uint32_t i = 0; i < mesh->vertex_count; i++) {
                        const glm::vec3& local_coord = glm::make_vec3(&mesh->vertices[i].pos.x) - (glm::vec3 { size } * 0.5f);
                        const glm::vec3& coord = transform.get_world_matrix() * glm::vec4 { local_coord * UNITS_PER_VOXEL, 1.0f };

                        // Invert the X-axis to correctly load it in external programs.
                        obj_file << std::format("v {} {} {}", coord.x, coord.y, -coord.z) << '\n';
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
                        const glm::vec3 albedo = material.albedo.unpack();

                        mtl_file << std::format("Kd {} {} {}", cs::linearize(albedo.r), cs::linearize(albedo.g), cs::linearize(albedo.b)) << '\n';
                        mtl_file << "d 1.0" << '\n';
                    }
                }
            }

            obj_file.close();
            mtl_file.close();
        },
        { { file_description.c_str(), file_extension.c_str() } }
    );
}

void NodeHierarchy::build_scene(const std::span<VoxelSceneNode>& root_nodes, bool assign_new_uuids, const std::map<UUID, Entity>& previous_entity_mapping) {
    for (const VoxelSceneNode& root_node : root_nodes) {
        const Entity root_entity = recurse_build_scene(root_node, assign_new_uuids, previous_entity_mapping);

        root_entities.push_back(root_entity);
    }
}

std::vector<char> NodeHierarchy::encode_voxel_scene() const {
    const auto& entities = engine.ecs.get_registry().storage<Transform>();
    if (entities.empty()) return {};  // Return empty scene encoding.

    std::vector<VoxelSceneNode> root_nodes;
    // For each root entity, recursively build a VoxelSceneNode of it and its children.
    for (const Entity entity : root_entities) {
        Transform& transform = engine.ecs.get_component<Transform>(entity);

        VoxelSceneNode& root_node = root_nodes.emplace_back();
        recurse_parse_scene(entity, transform, root_node);
    }

    // Encode those VoxelSceneNodes into the .svh format.
    return encode_svh(root_nodes);
}

void NodeHierarchy::recalculate_all_physics() {
    const entt::basic_group renderer_group = engine.ecs.group<VoxelRenderer>(entt::get<Transform>);

    for (const auto&& [entity, renderer, transform] : renderer_group.each()) {
        Physics::recalculate_surface_normals(*renderer.resource.resource);
    }
}

void NodeHierarchy::recurse_display_node(const Entity entity, const Name& name, Transform& transform, std::vector<Entity>& all_entities) {
    ImGui::PushID(static_cast<int>(entity));

    const std::set<Entity>& children = transform.get_children();

    constexpr ImGuiTreeNodeFlags default_flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DrawLinesToNodes |
                                                 ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    ImGuiTreeNodeFlags flags = default_flags;
    flags |= (children.empty() ? ImGuiTreeNodeFlags_Leaf : 0);
    flags |= (is_entity_selected(entity) ? ImGuiTreeNodeFlags_Selected : 0);

    ImGui::SetNextItemSelectionUserData(static_cast<ImGuiSelectionUserData>(all_entities.size()));
    all_entities.push_back(entity);

    const bool tree_open = ImGui::TreeNodeEx(name.name.c_str(), flags);
    node_context_menu(entity);
    drag_node(entity, name.name);
    drop_node(entity, transform);

    const bool popup_clicked_open = (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup));
    if (popup_clicked_open) set_selected_entity(entity);
    if (tree_open) {
        for (const Entity child : children) {
            auto&& [child_name, child_transform] = engine.ecs.get_component<Name, Transform>(child);
            recurse_display_node(child, child_name, child_transform, all_entities);
        }

        ImGui::TreePop();
    } else {
        const std::set<Entity> all_children = transform.get_all_children();
        all_entities.insert(all_entities.end(), all_children.begin(), all_children.end());
    }

    ImGui::PopID();
}

void NodeHierarchy::clear_hierarchy() {
    clear_selected_entities();
    for (const Entity root_entity : root_entities) {
        engine.ecs.destroy_entity(root_entity);
    }
    root_entities.clear();

    // Clear the undo/redo stack so the user doesn't try to undo changes to an old file in the newly loaded one.
    editor.systems[Editor::Mode::VOXEL].get<UndoRedoManager>().clear();
}

void NodeHierarchy::drop_hierarchy() {
    const ImVec2 min = ImGui::GetWindowPos() + ImGui::GetStyle().WindowPadding;
    const ImVec2 max = min + ImGui::GetWindowSize() - ImGui::GetStyle().WindowPadding;
    const ImRect window_rect { min, max };

    if (!ImGui::BeginDragDropTargetCustom(window_rect, ImGui::GetID("NodeEmptyDropArea"))) return;

    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("VoxelNode");
    if (payload != nullptr) {
        const Entity dropped_entity = *static_cast<Entity*>(payload->Data);
        Transform& dropped_transform = engine.ecs.get_component<Transform>(dropped_entity);

        if (dropped_transform.has_parent()) {
            // Diffs for parent child relationship undo/redo.
            ComponentDiff<Transform> parent_diff { dropped_transform.get_parent() };
            ComponentDiff<Transform> dropped_diff { dropped_entity };

            parent_diff.before();
            dropped_diff.before();

            dropped_transform.clear_parent();

            parent_diff.after();
            dropped_diff.after();

            UndoRedoCollection diff_collection;
            diff_collection.add_action(std::move(parent_diff));
            diff_collection.add_action(std::move(dropped_diff));

            NodeVectorDiff root_diff { root_entities };

            root_diff.before();
            root_entities.push_back(dropped_entity);
            root_diff.after();

            diff_collection.add_action(std::move(root_diff));
            diff_collection.commit("Modified Hierarchy");
        }
    }

    ImGui::EndDragDropTarget();
}

void NodeHierarchy::drop_node(const Entity entity, Transform& transform) {
    if (!ImGui::BeginDragDropTarget()) return;

    const ImGuiPayload* payload = ImGui::GetDragDropPayload();
    if (payload != nullptr && payload->IsDataType("VoxelNode")) {
        const Entity dropped_entity = *static_cast<Entity*>(payload->Data);

        if (entity != dropped_entity && ImGui::AcceptDragDropPayload("VoxelNode")) {
            const size_t child_count = transform.get_children().size();

            // Diffs for parent child relationship undo/redo.
            std::vector<ComponentDiff<Transform>> diffs;
            diffs.emplace_back(entity);
            diffs.emplace_back(dropped_entity);

            const Transform& dropped_transform = engine.ecs.get_component<Transform>(dropped_entity);
            if (dropped_transform.has_parent()) diffs.emplace_back(dropped_transform.get_parent());

            for (auto& diff : diffs) {
                diff.before();
            }

            transform.add_child(dropped_entity);

            // Only handle the rest of the diff process if the transform actually got a new child (might not get a new child when trying to parent to child of self).
            if (child_count != transform.get_children().size()) {
                UndoRedoCollection diff_collection;
                for (auto& diff : diffs) {
                    diff.after();
                    diff_collection.add_action(std::move(diff));
                }

                const auto iterator = std::ranges::find(root_entities, dropped_entity);
                if (iterator != root_entities.end()) {
                    // Diff to add entity back in root entities set.
                    NodeVectorDiff root_diff { root_entities };

                    root_diff.before();
                    root_entities.erase(iterator);
                    root_diff.after();

                    diff_collection.add_action(std::move(root_diff));
                }

                diff_collection.commit("Modified Hierarchy");
            }
        }
    }

    ImGui::EndDragDropTarget();
}

void NodeHierarchy::popup_create_node() {
    // Store the popup id so other functions can use it to open the popup.
    if (creation_popup_id == 0) creation_popup_id = ImGui::GetCurrentWindow()->GetID("Create New Node");

    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;
    if (!ImGui::BeginPopupModal("Create New Node", nullptr, flags)) return;

    ImGui::InputText("Node name", &node_creation_info->name);

    bool& has_voxel_grid = node_creation_info->has_voxel_grid;
    ImGui::Checkbox("Has voxel grid", &has_voxel_grid);

    if (has_voxel_grid) drag3_uint32("Size", &node_creation_info->voxel_size.x, 0.25f, 1, 1024);

    if (ImGui::Button("Create")) {
        const Entity new_node_entity = engine.ecs.create_entity(node_creation_info->name);

        if (node_creation_info->parent != entt::null)
            engine.ecs.get_component<Transform>(new_node_entity).set_parent(node_creation_info->parent);
        else
            root_entities.push_back(new_node_entity);

        UUID uuid;
        if (has_voxel_grid) {
            // Create a fake voxel resource managed by the voxel editor.
            const ResourceRef volume { {}, std::make_shared<VoxelVolume>(node_creation_info->voxel_size) };
            uuid = volume->uuid;
            engine.ecs.add_component<VoxelRenderer>(new_node_entity).resource = volume;
        }

        if (uuid == NULL_UUID) uuid = UUIDGenerator::generate();
        engine.ecs.add_component<NodeUUID>(new_node_entity).uuid = uuid;

        ImGui::CloseCurrentPopup();
        VoxelNodeDiff diff { new_node_entity, true };
        VoxelNodeDiff::send_to_manager(std::move(diff), "Added Voxel Node");

        node_creation_info.reset();
    }
    if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
        node_creation_info.reset();
    }

    ImGui::EndPopup();
}

void NodeHierarchy::popup_resize_node() {
    // Store the popup id so other functions can use it to open the popup.
    if (resize_popup_id == 0) resize_popup_id = ImGui::GetCurrentWindow()->GetID("Resize Node");

    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
    if (!ImGui::BeginPopupModal("Resize Node", nullptr, flags)) return;

    auto&& [name, transform] = engine.ecs.get_component<Name, Transform>(node_resize_info->entity);
    VoxelRenderer* renderer = engine.ecs.try_get_component<VoxelRenderer>(node_resize_info->entity);

    glm::uvec3 current_grid_size { 0u, 0u, 0u };
    if (renderer != nullptr) current_grid_size = renderer->resource->size;

    ImGui::Text("Node Name: %s", name.name.c_str());

    ImGui::BeginDisabled(true);
    drag3_uint32("Size", &current_grid_size.x);
    ImGui::EndDisabled();

    drag3_uint32("New Size", &node_resize_info->size.x, 0.1f, 0, 1024);

    constexpr glm::uvec3 min_offset { 0u };
    const glm::uvec3 max_offset { glm::abs(glm::i64vec3 { current_grid_size } - glm::i64vec3 { node_resize_info->size }) };
    slider3_uint32("Offset", &node_resize_info->offset.x, min_offset, max_offset);
    node_resize_info->offset = glm::clamp(node_resize_info->offset, min_offset, max_offset);

    ImGui::BeginDisabled(current_grid_size == node_resize_info->size);
    if (ImGui::Button("Resize")) {
        // Only rescale the volume if the new scale isn't 0 on any axis.
        if (node_resize_info->size.x != 0 && node_resize_info->size.y != 0 && node_resize_info->size.z != 0) {
            const auto new_volume = std::make_shared<VoxelVolume>(node_resize_info->size);
            const ResourceRef grid_resource { {}, new_volume };

            // If the node already has a renderer (aka: has a grid), copy over its data.
            if (renderer != nullptr) {
                // For each axis check if the current size is greater than the new size.
                const glm::bvec3 result = glm::greaterThan(current_grid_size, node_resize_info->size);

                // Using the greater than results we decided the min and max for each axis, these different depending on if we are decreasing or increasing the grid size.
                const glm::uvec3 min = glm::mix(glm::zero<glm::uvec3>(), node_resize_info->offset, result);
                const glm::uvec3 max = glm::mix(current_grid_size, node_resize_info->offset + node_resize_info->size, result);

                // Copy the pallet to the new volume.
                new_volume->blas->palette = renderer->resource->blas->palette;

                // Iterate over the voxels remaining in the model and set them in the new volume.
                for (uint32_t x = min.x; x < max.x; x++) {
                    for (uint32_t y = min.y; y < max.y; y++) {
                        for (uint32_t z = min.z; z < max.z; z++) {
                            const Material* material = renderer->resource->blas->get_voxel(x, y, z);

                            if (material == nullptr) continue;

                            const MaterialIndex index = static_cast<MaterialIndex>(material - renderer->resource->blas->palette.entries);

                            // Kind scuffed because we are taking the negative of an unsigned value, but this gets the start position in the new voxel grid.
                            const glm::uvec3 new_min = glm::mix(node_resize_info->offset, -min, result);
                            new_volume->blas->set_voxel(x + new_min.x, y + new_min.y, z + new_min.z, index);
                        }
                    }
                }
                new_volume->set_dirty();

                // Apply offsets to the entity and its children to match the visualization.
                const glm::vec3 half_extent = glm::vec3 { current_grid_size } * VOXEL_SIZE_HALF;
                const glm::vec3 resize_half_extent = glm::vec3 { node_resize_info->size } * VOXEL_SIZE_HALF;

                // Sign multiplication since we have to invert the offset when increasing the grid size.
                const glm::vec3 local_offset = glm::vec3 { node_resize_info->offset } * glm::sign(half_extent - resize_half_extent) * UNITS_PER_VOXEL;
                const glm::vec3 offset = (resize_half_extent - half_extent) + local_offset;

                transform.set_local_position(transform.get_local_position() + offset);

                for (const Entity child : transform.get_children()) {
                    Transform& child_transform = engine.ecs.get_component<Transform>(child);
                    child_transform.set_local_position(child_transform.get_local_position() - offset);
                }

                GridResizeDiff diff { node_resize_info->entity, offset, renderer->resource, grid_resource };
                GridResizeDiff::send_to_manager(std::move(diff), "Resize Grid");
            } else {
                // If the node doesn't already have a renderer (aka: doesn't have a grid), add it.
                renderer = &engine.ecs.add_component<VoxelRenderer>(node_resize_info->entity);

                const glm::vec3 resize_half_extent = glm::vec3 { node_resize_info->size } * VOXEL_SIZE_HALF;

                const glm::vec3 local_offset = -glm::vec3 { node_resize_info->offset } * UNITS_PER_VOXEL;
                const glm::vec3 offset = resize_half_extent + local_offset;

                transform.set_local_position(transform.get_local_position() + offset);

                for (const Entity child : transform.get_children()) {
                    Transform& child_transform = engine.ecs.get_component<Transform>(child);
                    child_transform.set_local_position(child_transform.get_local_position() - offset);
                }

                GridResizeDiff diff { node_resize_info->entity, offset, {}, grid_resource };
                GridResizeDiff::send_to_manager(std::move(diff), "Resize Grid");
            }
            renderer->resource = grid_resource;
        } else {
            renderer = &engine.ecs.get_component<VoxelRenderer>(node_resize_info->entity);
            GridResizeDiff diff { node_resize_info->entity, glm::zero<glm::vec3>(), renderer->resource, {} };
            GridResizeDiff::send_to_manager(std::move(diff), "Resize Grid");

            // Remove the VoxelRenderer component if the new grid size of 0 on any axis, this removes the grid from the entity entirely.
            engine.ecs.remove_component<VoxelRenderer>(node_resize_info->entity);
        }

        ImGui::CloseCurrentPopup();
        node_resize_info.reset();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
        node_resize_info.reset();
    }

    ImGui::EndPopup();
}

void NodeHierarchy::node_context_menu(const Entity node_entity) {
    constexpr ImGuiPopupFlags flags = ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_MouseButtonRight;
    if (!ImGui::BeginPopupContextItem(nullptr, flags)) return;

    if (ImGui::MenuItem(ICON_MS_ADD " Add Node")) {
        node_creation_info = std::make_unique<NodeCreationData>();
        node_creation_info->parent = node_entity;
        ImGui::OpenPopupEx(creation_popup_id);
    }
    if (ImGui::MenuItem(ICON_MS_REMOVE " Delete Node", "Delete")) {
        NodeVectorDiff selection_diff { selected_entities };
        selection_diff.before();
        selected_entities.clear();
        selection_diff.after();

        VoxelNodeDiff node_diff { node_entity, false };
        engine.ecs.destroy_entity(node_entity);

        UndoRedoCollection diff_collection;
        diff_collection.add_action(std::move(node_diff));
        diff_collection.add_action(std::move(selection_diff));
        diff_collection.commit("Deleted Voxel Node");
    }

    if (ImGui::MenuItem(ICON_MS_COPY_ALL " Duplicate Node", "Ctrl D")) {
        const Entity parent = engine.ecs.get_component<Transform>(node_entity).get_parent();
        const Entity new_entity = recurse_duplicate_node(node_entity, parent);
        if (parent == entt::null) root_entities.push_back(new_entity);

        VoxelNodeDiff diff { new_entity, true };
        VoxelNodeDiff::send_to_manager(std::move(diff), "Duplicate Voxel Node");
    }

    ImGui::Separator();

    if (ImGui::MenuItem(ICON_MS_ZOOM_OUT_MAP " Resize Grid", "C")) {
        node_resize_info = std::make_unique<NodeResizeData>();
        node_resize_info->entity = node_entity;

        const VoxelRenderer* renderer = engine.ecs.try_get_component<VoxelRenderer>(node_entity);
        if (renderer != nullptr) node_resize_info->size = renderer->resource->size;

        ImGui::OpenPopupEx(resize_popup_id);
    }

    ImGui::EndPopup();
}

void NodeHierarchy::on_inspect() {
    model_load_atomic.wait(false);

    popup_create_node();
    popup_resize_node();

    // Keyboard shortcuts
    if (ImGui::GetIO().WantCaptureKeyboard == false && selected_entities.empty() == false) {
        const Entity first_entity = selected_entities[0];

        // Delete shortcut
        if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
            NodeVectorDiff selection_diff { selected_entities };
            selection_diff.before();
            selected_entities.clear();
            selection_diff.after();

            VoxelNodeDiff node_diff { first_entity, false };
            engine.ecs.destroy_entity(first_entity);

            UndoRedoCollection diff_collection;
            diff_collection.add_action(std::move(node_diff));
            diff_collection.add_action(std::move(selection_diff));
            diff_collection.commit("Deleted Voxel Node");
        }

        // Duplicate shortcut
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_D)) {
            const Entity parent = engine.ecs.get_component<Transform>(first_entity).get_parent();
            const Entity new_entity = recurse_duplicate_node(first_entity, parent);
            selected_entities.clear();
            add_selected_entity(new_entity);
            if (parent == entt::null) root_entities.push_back(new_entity);

            VoxelNodeDiff diff { new_entity, true };
            VoxelNodeDiff::send_to_manager(std::move(diff), "Duplicate Voxel Node");
        }

        // Resize shortcut
        if (ImGui::IsKeyDown(ImGuiKey_C)) {
            node_resize_info = std::make_unique<NodeResizeData>();
            node_resize_info->entity = first_entity;

            const VoxelRenderer* renderer = engine.ecs.try_get_component<VoxelRenderer>(first_entity);
            if (renderer != nullptr) node_resize_info->size = renderer->resource->size;

            ImGui::OpenPopupEx(resize_popup_id);
        }
    }

    constexpr ImGuiMultiSelectFlags multiselect_flags =
        ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_ClearOnClickVoid | ImGuiMultiSelectFlags_BoxSelect1d | ImGuiMultiSelectFlags_SelectOnClickRelease;

    // Handle multi selecting entities at the start of the recursive display.
    std::vector<Entity> all_entities;

    // Built in multi select system in imgui.
    ImGuiMultiSelectIO* multi_select_io = ImGui::BeginMultiSelect(multiselect_flags, static_cast<int>(selected_entities.size()));
    // Check if ctrl+A is pressed to select all, this is the only selection event that happens during ImGui::BeginMultiSelect and not during ImGui::EndMultiSelect for some reason.
    const auto iterator =
        std::ranges::find_if(multi_select_io->Requests, [](const ImGuiSelectionRequest& request) { return request.Type == ImGuiSelectionRequestType_SetAll && request.Selected == true; });
    const bool set_all = iterator != multi_select_io->Requests.end();

    for (const Entity entity : root_entities) {
        auto&& [transform, name] = engine.ecs.get_component<Transform, Name>(entity);

        recurse_display_node(entity, name, transform, all_entities);
    }

    NodeVectorDiff diff { selected_entities };
    diff.before();

    // Handle the selection event request after all the items have been displayed.
    multi_select_io = ImGui::EndMultiSelect();
    apply_requests(multi_select_io, selected_entities, all_entities);
    if (set_all) selected_entities = all_entities;

    diff.after();
    if (diff.has_changed()) IUndoRedo::send_to_manager(std::move(diff), "Modified Node Selection");

    // Erase all root entities that have been marked for delete.
    std::erase_if(root_entities, [](const Entity& entity) { return !engine.ecs.valid(entity) || engine.ecs.has_component<Delete>(entity); });

    constexpr ImGuiPopupFlags flags = ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_MouseButtonRight;
    if (ImGui::BeginPopupContextWindow(nullptr, flags)) {
        if (ImGui::MenuItem(ICON_MS_ADD " Add node")) {
            node_creation_info = std::make_unique<NodeCreationData>();
            ImGui::OpenPopupEx(creation_popup_id);
        }
        ImGui::EndPopup();
    }

    drop_hierarchy();
}

void NodeHierarchy::on_editor_update(const FrameData&) {
    // If we aren't resizing we don't draw anything on update.
    if (node_resize_info == nullptr) return;

    glm::vec3 current_grid_size { 0u };
    const VoxelRenderer* renderer = engine.ecs.try_get_component<VoxelRenderer>(node_resize_info->entity);
    if (renderer != nullptr) current_grid_size = renderer->resource->size;

    const glm::vec3 half_extent = current_grid_size * VOXEL_SIZE_HALF;
    const glm::vec3 resize_half_extent = glm::vec3 { node_resize_info->size } * VOXEL_SIZE_HALF;

    const Transform& transform = engine.ecs.get_component<Transform>(node_resize_info->entity);

    // Sign multiplication since we have to invert the offset when increasing the grid size.
    const glm::vec3 local_offset = glm::vec3 { node_resize_info->offset } * glm::sign(half_extent - resize_half_extent) * UNITS_PER_VOXEL;

    engine.polyline.use_color(glm::vec3 { 1.0f, 0.0f, 0.0f });
    engine.polyline.draw_obb(transform.get_world_matrix() * glm::vec4 { (resize_half_extent - half_extent) + local_offset, 1.0f }, resize_half_extent, transform.get_world_rotation());
}

}  // namespace tmt