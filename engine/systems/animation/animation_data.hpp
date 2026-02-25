#pragma once

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/name.hpp"
#include "engine/core/resource.hpp"
#include "engine/core/resources.hpp"

#include <unordered_map>
#include "engine/core/reflection.hpp"

struct FBX;
struct ufbx_scene;
struct ufbx_node;

namespace tmt {

struct KeyframePos {
    float time = -1.0f;

    glm::vec3 position = glm::vec3(NAN);
};

struct KeyframeRot {
    float time = -1.0f;

    glm::quat rotation = glm::quat(NAN, 0, 0, 0);
};

struct KeyframeScale {
    float time = -1.0f;

    glm::vec3 scale = glm::vec3(NAN);
};

struct Animation {
    std::string name;
    std::vector<KeyframePos> keyframes_pos;
    std::vector<KeyframeRot> keyframes_rot;
    std::vector<KeyframeScale> keyframes_scale;
};

struct Bone {
    std::string name;
    float length = 0.0f;
    int index = -1;
    std::vector<int> children;
    Transform default_trans;
    glm::vec3 mesh_offset;
    glm::quat mesh_rot;
    glm::vec3 world_pos;
    std::unordered_map<std::string, Animation> animations;
};

struct RigData : public FileResource {
   public:
    RigData(const IO::FileLocation& directory);

    // Inherited via Resource
    bool load() override;
    void unload() override;
    bool reload() override;

    inline static const std::set<std::string_view> SUPPORTED_FILE_EXTENSIONS { ".fbx" };

    std::vector<Bone> bones;
    std::string name;

    // Hacky way of ding this, this info should not be stored in the resources since it can not be found in the .FBX file itself
    // std::vector<std::pair<FileIO::Directory, std::string>> animation_files;
    //std::vector<IO::FileLocation> animation_files;
    std::unordered_map<IO::FileLocation, std::string> animation_files;

   private:
    void extract_bone_keyframes_fbx(const ufbx_scene* animation_fbx, const ufbx_scene* rig_fbx, const std::map<const ufbx_node*, const ufbx_node*>& mesh_connections);
    void init_bone_fbx(const ufbx_node* node, const std::map<const ufbx_node*, const ufbx_node*>& mesh_connections);
};

}  // namespace tmt

TMT_OBJECT(tmt::RigData, (animation_files));