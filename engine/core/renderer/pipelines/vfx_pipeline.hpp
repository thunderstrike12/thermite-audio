#pragma once

#include <graphite/resources/handle.hh>

#include "engine/core/renderer/render_view.hpp"
#include "engine/core/components/emitter.hpp"

class GPUAdapter;
class RenderGraph;

namespace tmt {

constexpr uint32_t MAX_PARTICLES = 100'000;
constexpr uint32_t MAX_EMITTERS = 64;
constexpr uint32_t MAX_EFFECTS_PER_EMITTER = 8;

struct GpuQuad {
    glm::vec2 pos {};
    glm::vec2 uvs {};
};

struct GpuBezierCurve {
    glm::vec4 points {};
};

struct GpuParticleEffect {
    glm::vec3 pos {};
    float lifetime {};

    glm::vec3 dir {};
    float cone_angle {};

    Range start_speed {};
    Range end_speed {};
    GpuBezierCurve speed_curve {};

    Range start_size {};
    Range end_size {};
    GpuBezierCurve size_curve {};

    Range start_opacity {};
    Range end_opacity {};
    GpuBezierCurve opacity_curve {};

    Range rotation {};
    Range pos_jitter {};

    float jitter_speed {};
    uint32_t flipbook_frames {};
    float anim_speed {};
    float dither_scale {};

    uint32_t spawn_count {};
    uint32_t tex_index {};
    glm::vec2 pad {};
};

struct GpuEmitter {
    uint32_t effects_offset {};
    uint32_t effects_count {};
    uint32_t total_spawn_count {};
};

struct GpuParticle {
    glm::vec3 pos {};
    float age {};

    glm::vec3 start_speed {};
    float pad {};
    glm::vec3 end_speed {};
    float pad2 {};

    glm::vec3 dir {};
    float lifetime {};

    float size {};
    float start_size {};
    float end_size {};
    float opacity {};

    float start_opacity {};
    float end_opacity {};
    float rot_speed {};
    float rotation {};

    GpuBezierCurve size_curve {};
    GpuBezierCurve opacity_curve {};
    GpuBezierCurve speed_curve {};

    float pos_jitter {};
    uint32_t tex_index {};
    uint32_t effect_index {};
    uint32_t current_frame {};

    uint32_t flipbook_frames {};
    float anim_speed {};
    float dither_scale {};
    float pad3 {};

    glm::vec3 prev_pos {};
    float pad4 {};
};

struct Counters {
    uint32_t alive_count {};
    uint32_t alive_count_after_sim {};
    uint32_t dead_count {};
    uint32_t draw_count {};  // Amount of particles to be drawn
};

struct Billboard {
    glm::vec4 color {};
    glm::vec3 pos {};
    float size {};
};

class VfxPipeline {
   public:
    VfxPipeline() {}
    ~VfxPipeline() {}

    VfxPipeline(const VfxPipeline&) = delete;
    VfxPipeline& operator=(const VfxPipeline&) = delete;

    void init(GPUAdapter& gpu);
    void enqueue(RenderGraph& render_graph, RenderView render_view);
    void deinit(GPUAdapter& gpu);

    uint32_t emitter_count = 0u;
    uint32_t effects_count = 0u;

    /* Buffers */
    Buffer counter_buffer {};
    Buffer alive_list {};
    Buffer alive_list_new {};
    Buffer dead_list {};
    Buffer particle_buffer {};
    Buffer dispatch_args_buffer {};
    Buffer instanced_draw_args_buffer {};
    Buffer emitter_buffer {};
    Buffer particle_effects_buffer {};
    Buffer billboard_vertices {};

    Sampler point_sampler {};
};

}  // namespace tmt