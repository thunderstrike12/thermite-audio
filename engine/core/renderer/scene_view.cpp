#include "scene_view.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/render_graph.hh>

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/renderer/voxel_object.hpp"

#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/components/light.hpp"
#include "engine/core/components/environment.hpp"

namespace tmt {

/* Universal light descriptor. */
struct UniversalLightDesc {
    /* The kind of light source. */
    LightType light_type {};
    /* Origin point of the light source. */
    glm::vec3 origin {};

    /* Exitent luminance of the light source. (in candela) */
    glm::vec3 luminance {};
    /* Radius of the light source, defines the softness of its shadows. */
    float source_radius = 0.0f;

    /* Radius/distance at which the emitted light influence reaches zero. */
    float attenuation_radius = 1e30f;
    /* Radius/distance at which the light source will be culled. */
    float culling_radius = 1e30f;
    /* Cosine of the angular radius of the spot/sun light beam. */
    float culling_angle = -1.0f;
    /* The softness of the spot light edge. (0..1) */
    float spot_blend = 0.0f;

    /* Primary direction of the light source. */
    glm::vec3 direction = glm::vec3(0.0f, 1.0f, 0.0f);
    /* Length of the light source. (used for tube lights) */
    float source_length = 0.0f;
};

void SceneView::init() {
    /* Get the VRAM bank */
    VRAMBank& bank = engine.renderer.vram_bank();

    /* Create GPU resources */
    const BufferUsage s = BufferUsage::Storage | BufferUsage::TransferDst;
    const BufferUsage c = BufferUsage::Constant | BufferUsage::TransferDst;
    bvh_nodes = bank.create_buffer("BVH Nodes Buffer", s, MAX_VOXEL_OBJECTS * 2u + 1u, sizeof(AilaLaineNode)).expect("failed to create bvh nodes buffer.");
    object_indices = bank.create_buffer("Object Indices Buffer", s, MAX_VOXEL_OBJECTS, sizeof(uint32_t)).expect("failed to create object indices buffer.");
    object_data = bank.create_buffer("Object Data Buffer", s, MAX_VOXEL_OBJECTS, sizeof(GpuVoxelObject)).expect("failed to create object data buffer.");
    lights_data = bank.create_buffer("Lights Data Buffer", s, MAX_LIGHTS, sizeof(UniversalLightDesc)).expect("failed to create lights data buffer.");
    scene_view = bank.create_buffer("Scene View Buffer", c, sizeof(GpuSceneView)).expect("failed to create scene view buffer.");

    /* Subscribe to EnTT */
    engine.ecs.get_registry().on_destroy<VoxelRenderer>().connect<&SceneView::on_renderer_destroyed>(this);
    next_uuid = 1u; /* Set the base uuid */
}

void SceneView::update(RenderGraph& render_graph, const RenderView& render_view) {
    /* Update voxel objects */
    update_voxel_objects(render_graph);

    /* Update lights */
    update_lights(render_graph, render_view);
}

void SceneView::deinit() {
    /* Get the VRAM bank */
    VRAMBank& bank = engine.renderer.vram_bank();

    /* Destroy GPU resources */
    bank.destroy(bvh_nodes);
    bank.destroy(object_indices);
    bank.destroy(object_data);
    bank.destroy(lights_data);
    bank.destroy(scene_view);
}

/* Create a vector containing type `T`, with space reserved for `count` instances. */
template <typename T>
inline std::vector<T> reserved(const size_t count) {
    std::vector<T> v {};
    v.reserve(count);
    return std::move(v);
}

/* Validate a transform. */
bool validate_transform(const glm::mat4 m) {
    /* Calculate the scale of the transform */
    const glm::vec3 scale = glm::vec3(glm::length(glm::vec3(m[0])), glm::length(glm::vec3(m[1])), glm::length(glm::vec3(m[2])));

    /* Ensure the scale isn't too small */
    if (glm::any(glm::epsilonEqual(scale, glm::vec3(0.0f), 0.001f))) return true;

    /* Check for NaN and Inf values in the matrix */
    const glm::vec4 sum4 = m[0] + m[1] + m[2] + m[3];
    const float sum = sum4[0] + sum4[1] + sum4[2] + sum4[3];
    return glm::isnan(sum) || glm::isinf(sum);
}

void SceneView::update_voxel_objects(RenderGraph& render_graph) {
    /* Capture all voxel renderers in the scene */
    const entt::basic_group group = engine.ecs.group<VoxelRenderer>(entt::get<Transform>);

    /* Iterate over all voxel renderers */
    size_t uuid_count = (size_t)next_uuid;
    for (auto&& [entity, renderer, transform] : group.each()) {
        if (uuid_count >= (size_t)MAX_VOXEL_OBJECTS) break;
        if (renderer.uuid == 0u) uuid_count++;
    }

    /* Allocate space for all voxel objects */
    std::vector cpu_objects = std::vector<VoxelObject>(uuid_count);
    std::vector gpu_objects = std::vector<GpuVoxelObject>(uuid_count);
    entities = std::vector<Entity>(uuid_count);
    render_outlines = false;

    /* Iterate over all voxel renderers */
    for (auto&& [entity, renderer, transform] : group.each()) {
        /* Don't render objects with a null resource */
        if (renderer.resource == nullptr) continue;

        /* Don't render objects with invalid transforms */
        if (validate_transform(transform.get_world_matrix())) continue;
        renderer.resource->update_if_dirty();

        /* Set the UUID of the object */
        if (renderer.uuid == 0u) {
            if (uuid_free_list.empty()) {
                if (next_uuid >= MAX_VOXEL_OBJECTS) continue;
                renderer.uuid = next_uuid;
                next_uuid++;
            } else {
                renderer.uuid = uuid_free_list.back();
                uuid_free_list.pop_back();
            }
        }

        /* Create new CPU and GPU object */
        VoxelObject& cpu_object = cpu_objects[renderer.uuid];
        GpuVoxelObject& gpu_object = gpu_objects[renderer.uuid];
        cpu_object.uuid = renderer.uuid;

        /* Shared data */
        cpu_object.local_to_world = gpu_object.local_to_world = transform.get_world_matrix();
        /* Check if the entity had a transform last frame, if not use current frame matrix */
        gpu_object.prev_local_to_world = prev_transforms.contains(entity) ? prev_transforms.at(entity) : gpu_object.local_to_world;
        cpu_object.world_to_local = gpu_object.world_to_local = glm::inverse(cpu_object.local_to_world);
        cpu_object.size = gpu_object.size = renderer.resource->size;
        cpu_object.rcp_tree_width = gpu_object.rcp_tree_width = 1.0f / powf(4.0f, (float)renderer.resource->blas->depth);

        /* CPU-only data */
        cpu_object.volume = renderer.resource.resource.get();

        /* GPU-only data */
        gpu_object.tree_depth = renderer.resource->blas->depth;
        gpu_object.blas_handle = renderer.resource->blas_nodes.get_index();
        gpu_object.voxels_handle = renderer.resource->blas_voxels.get_index();
        gpu_object.palette_handle = renderer.resource->blas_palette.get_index();
        gpu_object.object_flags = renderer.outlined ? 0b1u : 0b0u;
        if (renderer.outlined) render_outlines = true;
        renderer.outlined = false; /* Reset outlined flag */

        /* Save the entity id */
        entities[renderer.uuid] = entity;

        /* Insert Entity Transform */
        prev_transforms[entity] = gpu_object.local_to_world;
    }

    /* Build a BVH over the scene */
    bvh.build(cpu_objects.data(), (uint32_t)cpu_objects.size());

    /* Upload the BVH buffers */
    render_graph.upload_buffer(bvh_nodes, bvh.gpu_nodes, 0u, bvh.node_count * sizeof(AilaLaineNode));
    render_graph.upload_buffer(object_indices, bvh.indices, 0u, bvh.index_count * sizeof(uint32_t));
    render_graph.upload_buffer(object_data, gpu_objects.data(), 0u, gpu_objects.size() * sizeof(GpuVoxelObject));
}

void SceneView::update_lights(RenderGraph& render_graph, const RenderView&) {
    /* Capture all lights in the scene */
    const entt::basic_group light_group = engine.ecs.group<const Light>(entt::get<Transform>);

    /* Allocate space for all lights */
    const size_t count = std::min(light_group.size(), (size_t)MAX_LIGHTS);
    std::vector gpu_lights = reserved<UniversalLightDesc>(count);
    GpuSceneView gpu_view {};

    /* Iterate over all lights */
    for (auto&& [entity, light, transform] : light_group.each()) {
        const glm::vec3 scale = transform.get_world_scale();

        /* We only support 1 sun light in the scene at once */
        if (light.type == LightType::SUN_LIGHT) {
            const SunLight sun_light = std::get<SunLight>(light.light);
            gpu_view.sun_angle = glm::cos(sun_light.source_angle);
            gpu_view.sun_dir = -transform.get_forward();
            gpu_view.sun_luminance = light.calculate_luminance(scale);
            continue;
        }

        /* Create a new universal light descriptor */
        UniversalLightDesc& gpu_light = gpu_lights.emplace_back();
        gpu_light.light_type = light.type;
        gpu_light.origin = transform.get_world_position();
        gpu_light.direction = transform.get_forward();
        gpu_light.luminance = light.calculate_luminance(scale);

        switch (light.type) {
            /* Spherical area light */
            case LightType::SPHERE_LIGHT: {
                const SphereLight sphere_light = std::get<SphereLight>(light.light);
                gpu_light.source_radius = sphere_light.source_radius;
                gpu_light.attenuation_radius = sphere_light.attenuation_radius;
                gpu_light.culling_radius = sphere_light.attenuation_radius;
                break;
            }
            /* Spot light */
            case LightType::SPOT_LIGHT: {
                const SpotLight spot_light = std::get<SpotLight>(light.light);
                gpu_light.source_radius = spot_light.source_radius;
                gpu_light.attenuation_radius = spot_light.attenuation_distance;
                gpu_light.culling_angle = glm::cos(spot_light.beam_angle * 0.5f);
                gpu_light.spot_blend = spot_light.spot_blend;
                gpu_light.culling_radius = spot_light.attenuation_distance;
                break;
            }
            /* Spot light */
            case LightType::TUBE_LIGHT: {
                const TubeLight tube_light = std::get<TubeLight>(light.light);
                gpu_light.source_radius = tube_light.source_radius;
                gpu_light.attenuation_radius = tube_light.attenuation_distance;
                gpu_light.source_length = scale.z;
                gpu_light.culling_radius = scale.z * 0.5f + tube_light.attenuation_distance;
                break;
            }
            default:
                break;
        }
    }

    /* Capture all environment in the scene */
    const entt::basic_group env_group = engine.ecs.group<const Environment>();
    gpu_view.envmap_full_handle = 0u;
    gpu_view.envmap_filtered_handle = 0u;

    /* Iterate over all environments */
    for (auto&& [entity, env] : env_group.each()) {
        if (env.resource) {
            /* Select the first valid environment we find */
            gpu_view.envmap_full_handle = env.resource->full_image.get_index();
            gpu_view.envmap_filtered_handle = env.resource->filtered_image.get_index();
            break;
        }
    }

    /* Set the light count on the GPU view */
    gpu_view.light_count = (uint32_t)gpu_lights.size();

    /* Upload the light buffers */
    render_graph.upload_buffer(lights_data, gpu_lights.data(), 0u, gpu_lights.size() * sizeof(UniversalLightDesc));
    render_graph.upload_buffer(scene_view, &gpu_view, 0u, sizeof(GpuSceneView));
}

void SceneView::on_renderer_destroyed(entt::registry& registry, entt::entity entity) {
    /* Add now unused uuids to the free list */
    VoxelRenderer* voxel_renderer = registry.try_get<VoxelRenderer>(entity);
    if (voxel_renderer != nullptr) {
        uuid_free_list.push_back(voxel_renderer->uuid);
    }

    /* Remove entity from previous transforms */
    prev_transforms.erase(entity);
}

}  // namespace tmt
