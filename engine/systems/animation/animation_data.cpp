#include "animation_data.hpp"

#include "ufbx.h"
#include "glm/gtc/type_ptr.hpp"

namespace {

// from tools.cpp in kudzu
// Courtesy of: http://stackoverflow.com/questions/5878775/how-to-find-and-replace-string
std::string string_replace(const std::string& subject, const std::string& search, const std::string& replace) {
    std::string result(subject);
    size_t pos = 0;

    while ((pos = subject.find(search, pos)) != std::string::npos) {
        result.replace(pos, search.length(), replace);
        pos += search.length();
    }

    return result;
}

// Check if a rig node is equal to and animation node.
bool check_correct_bone(std::string_view rig_bone, std::string_view animation_bone) {
    if (const size_t pos = rig_bone.find_last_of(':'); pos != std::string_view::npos) {
        rig_bone = rig_bone.substr(pos + 1);
    }
    if (const size_t pos = animation_bone.find_last_of(':'); pos != std::string_view::npos) {
        animation_bone = animation_bone.substr(pos + 1);
    }

    return rig_bone == animation_bone;
}

}  // namespace

namespace tmt {

RigData::RigData(const IO::FileLocation& directory) : FileResource(directory) {}

bool RigData::load() {
    const std::vector<char>& rig = IO::read_file(file_location);

    ufbx_error error;
    ufbx_load_opts opts = {};
    opts.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
    opts.target_axes = ufbx_axes_left_handed_y_up;
    opts.handedness_conversion_axis = UFBX_MIRROR_AXIS_Z;
    opts.evaluate_skinning = true;
    ufbx_scene* rig_fbx = ufbx_load_memory(rig.data(), rig.size(), &opts, &error);

    if (error.type != UFBX_ERROR_NONE) {
        Log::error("Failed to load FBX rig: {}\n", error.description.data);
        return false;
    }

    name = "Armature";

    std::map<const ufbx_node*, const ufbx_node*> mesh_connections;
    for (const ufbx_node* node : rig_fbx->nodes) {
        if (node->mesh == nullptr || node->mesh->skin_deformers.count <= 0) continue;

        const auto* bone_node = node->mesh->skin_deformers[0]->clusters[0]->bone_node;
        mesh_connections[bone_node] = node;
    }

    // Use std::find to find the node with the root bone (to start off our bone initialization)
    const ufbx_bone* root_bone = rig_fbx->bones[0];
    const ufbx_node* root_node = *std::ranges::find_if(rig_fbx->nodes, [&root_bone](const ufbx_node* bone_node) { return bone_node->bone == root_bone; });

    init_bone_fbx(root_node, mesh_connections);
    extract_bone_keyframes_fbx(rig_fbx, rig_fbx, mesh_connections);

    for (auto& [animation_directory, anim_name] : animation_files) {
        opts = {};
        opts.ignore_geometry = true;
        const std::vector<char>& animation_data = IO::read_file(animation_directory);
        ufbx_scene* animation_fbx = ufbx_load_memory(animation_data.data(), animation_data.size(), &opts, &error);

        if (error.type != UFBX_ERROR_NONE) {
            Log::error("Failed to load FBX animation: {}\n", error.description.data);
            continue;
        }

        extract_bone_keyframes_fbx(animation_fbx, rig_fbx, mesh_connections);

        ufbx_free_scene(animation_fbx);
    }

    ufbx_free_scene(rig_fbx);

    return true;
}

void tmt::RigData::unload() {}

bool tmt::RigData::reload() {
    bones.clear();

    return load();
}

void RigData::extract_bone_keyframes_fbx(const ufbx_scene* animation_fbx, const ufbx_scene* rig_fbx, const std::map<const ufbx_node*, const ufbx_node*>& mesh_connections) {
    for (const ufbx_anim_stack* animation : animation_fbx->anim_stacks) {
        const ufbx_baked_anim* baked = ufbx_bake_anim(animation_fbx, animation->anim, nullptr, nullptr);

        for (const ufbx_baked_node& bake_node : baked->nodes) {
            const ufbx_node* bone_node = animation_fbx->nodes[bake_node.typed_id];
            if (bone_node->bone == nullptr) continue;

            std::function<bool(const Bone&)> find_bone_predicate;

            // Get the bone node of the rig from the animation fbx file, since we are animating based on rig_fbx names.
            const auto animation_bone_node = std::ranges::find_if(rig_fbx->nodes.begin(), rig_fbx->nodes.end(), [bone_node](const ufbx_node* rig_node) {
                return check_correct_bone(rig_node->name.data, bone_node->name.data);
            });

            if (animation_bone_node == rig_fbx->nodes.end()) continue;

            bone_node = *animation_bone_node;

            const auto connection = mesh_connections.find(bone_node);
            if (connection != mesh_connections.end()) {
                find_bone_predicate = [connection](const Bone& rig_bone) { return check_correct_bone(rig_bone.name, connection->second->name.data); };
            } else {
                find_bone_predicate = [bone_node](const Bone& rig_bone) { return check_correct_bone(rig_bone.name, bone_node->name.data); };
            }

            const auto& bone_iterator = std::ranges::find_if(bones, find_bone_predicate);
            if (bone_iterator == bones.end()) continue;

            const std::filesystem::path generic_path = string_replace(animation_fbx->metadata.original_file_path.data, "\\", "/");
            const std::string animation_name = generic_path.stem().generic_string();

            auto& bone_comp = *bone_iterator;

            Animation& bone_animation = bone_comp.animations[animation_name];
            bone_animation.name = animation_name;

            for (const ufbx_baked_vec3& translation : bake_node.translation_keys) {
                // Flip z axis to convert coordinate system.
                bone_animation.keyframes_pos.emplace_back(static_cast<float>(translation.time), glm::vec3 { translation.value.x, translation.value.y, -translation.value.z });
            }

            for (const ufbx_baked_quat& rotation : bake_node.rotation_keys) {
                // Flip quaternion axes to convert coordinate system.
                bone_animation.keyframes_rot.emplace_back(static_cast<float>(rotation.time), glm::quat { rotation.value.w, -rotation.value.x, -rotation.value.y, rotation.value.z });
            }

            for (const ufbx_baked_vec3& scale : bake_node.scale_keys) {
                // Don't flip scale, the scale shouldn't be mirrored.
                bone_animation.keyframes_scale.emplace_back(static_cast<float>(scale.time), glm::make_vec3(scale.value.v));
            }
        }
    }
}

void RigData::init_bone_fbx(const ufbx_node* node, const std::map<const ufbx_node*, const ufbx_node*>& mesh_connections) {
    Bone bone;
    bone.name = node->name.data;

    const ufbx_transform& node_transform = node->local_transform;
    bone.default_trans.set_local_position(glm::make_vec3(node_transform.translation.v));
    bone.default_trans.set_local_rotation(glm::make_quat(node_transform.rotation.v));
    bone.default_trans.set_local_scale(glm::make_vec3(node_transform.scale.v));

    const auto mesh_connection = mesh_connections.find(node);
    if (mesh_connection != mesh_connections.end()) {
        const ufbx_node* mesh_node = mesh_connection->second;
        const ufbx_mesh* mesh = mesh_node->mesh;

        glm::vec3 min { 1e30f };
        glm::vec3 max { -1e30f };

        for (const auto& vertex : mesh->skinned_position.values) {
            const glm::vec3 vertex_position { vertex.x, vertex.y, vertex.z };
            min = glm::min(min, vertex_position);
            max = glm::max(max, vertex_position);
        }

        bone.mesh_offset = (min + max) * 0.5f;
        bone.name = mesh_node->name.data;
    }

    const int children_count = static_cast<int>(node->children.count);
    for (int i = 0; i < children_count; i++) {
        init_bone_fbx(node->children[i], mesh_connections);
        bone.children.push_back(static_cast<int>(bones.size() - 1));
    }

    bone.index = static_cast<int>(bones.size());
    bones.push_back(bone);
}

}  // namespace tmt
