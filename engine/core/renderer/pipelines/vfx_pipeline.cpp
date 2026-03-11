#include "vfx_pipeline.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>
#include <graphite/nodes/raster_node.hh>
#include <graphite/nodes/compute_node.hh>

#include "engine.hpp"
#include "core/ecs.hpp"
#include "core/logger.hpp"
#include "core/renderer/renderer.hpp"
#include "core/components/emitter.hpp"

namespace tmt {

void VfxPipeline::init(GPUAdapter& gpu) {
    VRAMBank& bank = gpu.get_vram_bank();

    /* Initialize the Counters Buffer */
    {
        counter_buffer =
            bank.create_buffer("[Particles] Counters Buffer", BufferUsage::Storage | BufferUsage::TransferDst, 1, sizeof(Counters)).expect("failed to initialise the counters buffer.");

        Counters counters = {};
        counters.alive_count = 0;
        counters.alive_count_after_sim = 0;
        counters.dead_count = MAX_PARTICLES;
        counters.draw_count = 0;

        bank.upload_buffer(counter_buffer, &counters, 0, sizeof(Counters));
    }

    /* Initialize the Alive Lists */
    {
        alive_list =
            bank.create_buffer("[Particles] Alive List", BufferUsage::Storage | BufferUsage::TransferDst, MAX_PARTICLES, sizeof(uint32_t)).expect("failed to initialise the alive list.");

        alive_list_new = bank.create_buffer("[Particles] Alive List New", BufferUsage::Storage | BufferUsage::TransferDst, MAX_PARTICLES, sizeof(uint32_t))
                             .expect("failed to initialise the alive list new.");

        uint32_t* arr = new uint32_t[MAX_PARTICLES];
        size_t size = sizeof(uint32_t) * MAX_PARTICLES;
        memset(arr, 0, size);

        bank.upload_buffer(alive_list, arr, 0, size);
        bank.upload_buffer(alive_list_new, arr, 0, size);
    }

    /* Initialize the Dead List */
    {
        dead_list = bank.create_buffer("[Particles] Dead List", BufferUsage::Storage | BufferUsage::TransferDst, MAX_PARTICLES, sizeof(uint32_t)).expect("failed to initialise the dead list.");

        uint32_t* arr = new uint32_t[MAX_PARTICLES];
        for (uint32_t i = 0; i < MAX_PARTICLES; i++) {
            arr[i] = i;
        }
        bank.upload_buffer(dead_list, arr, 0, sizeof(uint32_t) * MAX_PARTICLES);
    }

    /* Initialize the Particle Buffer */
    {
        particle_buffer = bank.create_buffer("[Particles] Particle Buffer", BufferUsage::Storage | BufferUsage::TransferDst, MAX_PARTICLES, sizeof(GpuParticle))
                              .expect("failed to initialise the particle buffer.");
    }

    /* Initialise the Indirect Buffers */
    {
        struct IndirectDispatchArgs {
            uint32_t x;
            uint32_t y;
            uint32_t z;
        };

        dispatch_args_buffer = bank.create_buffer("[Particles] Indirect Dispatch Buffer", BufferUsage::Storage | BufferUsage::Indirect, sizeof(IndirectDispatchArgs))
                                   .expect("failed to initialise the indirect dispatch args buffer.");

        struct IndirectDrawArgs {
            uint32_t vertex_count;
            uint32_t instance_count;
            uint32_t first_vertex;
            uint32_t first_instance;
        };

        instanced_draw_args_buffer = bank.create_buffer("[Particles] Indirect Draw Buffer", BufferUsage::Storage | BufferUsage::Indirect, sizeof(IndirectDrawArgs))
                                         .expect("failed to initialise the indirect draw args buffer.");
    }

    /* Initialise Emitter Buffer */
    {
        emitter_buffer = bank.create_buffer("[Particles] Emitter Buffer", BufferUsage::Storage | BufferUsage::TransferDst, MAX_EMITTERS, sizeof(GpuEmitter))
                             .expect("failed to initialise the particle emitter buffer.");
    }

    /* Initialise Particle Effects Buffer */
    {
        particle_effects_buffer =
            bank.create_buffer("[Particles] Particle Effects Buffer", BufferUsage::Storage | BufferUsage::TransferDst, MAX_EMITTERS * MAX_EFFECTS_PER_EMITTER, sizeof(GpuParticleEffect))
                .expect("failed to initialise the particle effects buffer.");
    }

    /* Initialize the Billboard Vertex Buffer */
    {
        billboard_vertices = bank.create_buffer("[Particles] Billboard Vertex Buffer", BufferUsage::Vertex | BufferUsage::TransferDst, 6, sizeof(GpuQuad))
                                 .expect("failed to initialise billboard vertex buffer.");

        /* clang-format off */
        GpuQuad quads[6] = { {{-0.5f, -0.5f}, {0.0f, 1.0f}},
                             {{0.5f, -0.5f},  {1.0f, 1.0f}},
                             {{0.5f, 0.5f},   {1.0f, 0.0f}},
                             {{-0.5f, -0.5f}, {0.0f, 1.0f}},
                             {{0.5f, 0.5f},   {1.0f, 0.0f}},
                             {{-0.5f, 0.5f},  {0.0f, 0.0f}},
                            };

        /* clang-format on */

        bank.upload_buffer(billboard_vertices, &quads, 0, sizeof(GpuQuad) * 6);
    }

    /* Initialize the Sampler */
    point_sampler = bank.create_sampler("[Particles] Linear Sampler", Filter::Nearest).expect("failed to initialize linear sampler.");
}

void VfxPipeline::enqueue(RenderGraph& render_graph, RenderView render_view) {
    const BindHandle render_image = render_view.get_render_image();
    std::swap(alive_list, alive_list_new);

    std::vector<GpuEmitter> emitters {};
    std::vector<GpuParticleEffect> effects {};

    /* clang-format off */
    const entt::basic_group group = engine.ecs.group<ParticleEmitter>(entt::get<Transform>);
    for (auto&& [entity, emitter, transform] : group.each()) {
        /* Skip inactive emitters */
        if (!emitter.active && !emitter.should_burst) continue;

        if (emitters.size() >= MAX_EMITTERS) break;

        if (effects.size() >= MAX_EMITTERS * MAX_EFFECTS_PER_EMITTER) break;

        emitter.should_burst = !emitter.should_burst;

        GpuEmitter em {};
        em.effects_count = static_cast<uint32_t>(emitter.effects.size());
        em.effects_offset = static_cast<uint32_t>(effects.size());

        for (auto& effect : emitter.effects) {
            if (!effect.active && !effect.should_burst) continue;

            effect.should_burst = !effect.should_burst;

            uint32_t actual_spawn_count = 0u;

            GpuParticleEffect eff {};

            if (effect.spawn_interval <= 0.0f) {
                // No interval so we spawn every frame
                actual_spawn_count = effect.spawn_count;
            } 
            else {
                effect.spawn_timer += render_view.gpu_view.dt;
                while (effect.spawn_timer >= effect.spawn_interval) {
                    actual_spawn_count += effect.spawn_count;
                    effect.spawn_timer -= effect.spawn_interval;
                }
            }

            // Convert from deg to dir vector
            const glm::quat local_rot = glm::quat(glm::radians(effect.dir));
            const glm::vec3 local_dir = local_rot * glm::vec3(0.0f, 0.0f, 1.0f);

            // Get emitter's world rotation
            const glm::quat world_rot = transform.get_world_rotation();

            eff.pos = transform.get_world_position() + world_rot * effect.pos_offset;
            eff.dir = glm::normalize(world_rot * local_dir);
            eff.cone_angle = glm::cos(glm::radians(effect.cone_angle));
            eff.start_speed = effect.start_speed;
            eff.end_speed = effect.end_speed;
            eff.lifetime = effect.particle_lifetime;
            eff.spawn_count = actual_spawn_count;
            eff.start_size = effect.start_size;
            eff.end_size = effect.end_size;
            eff.size_curve.points = effect.size_curve.get_vec4();
            eff.start_opacity = effect.start_opacity;
            eff.end_opacity = effect.end_opacity;
            eff.opacity_curve.points = effect.opacity_curve.get_vec4();
            eff.rotation = effect.rotation;
            eff.pos_jitter = effect.pos_jitter;
            eff.jitter_speed = effect.jitter_speed;
            eff.tex_index = effect.texture.resource->image.get_index();
            eff.flipbook_frames = effect.texture.resource->flipbook_frames;
            eff.anim_speed = effect.anim_speed;
            eff.dither_scale = effect.dither_scale;

            em.total_spawn_count += eff.spawn_count;

            effects.push_back(eff);
        }
        emitters.push_back(em);
    }

    render_graph.upload_buffer(emitter_buffer, emitters.data(), 0, sizeof(GpuEmitter) * emitters.size());
    render_graph.upload_buffer(particle_effects_buffer, effects.data(), 0, sizeof(GpuParticleEffect) * effects.size());

    /* Emit Stage */
    for (uint32_t i = 0; i < emitters.size(); i++) {
        const GpuEmitter& em = emitters[i];

        render_graph.add_compute_pass("Emit Particles", "emit.cs")
                .push_constants(&i, 0, sizeof(i))
                .read(render_view.render_view_buffer)
                .read(emitter_buffer)
                .read(particle_effects_buffer)
                .write(counter_buffer)
                .write(alive_list)
                .write(alive_list_new)
                .write(dead_list)
                .write(particle_buffer)
                .group_size(64, 1, 1)
                .work_size(em.total_spawn_count, 1, 1);
    }

    render_graph.add_compute_pass("Particle Begin Update", "particle_begin_update.cs")
                .write(counter_buffer)
                .write(dispatch_args_buffer)
                .group_size(1, 1, 1)
                .work_size(1, 1, 1);

    render_graph.add_compute_pass("Simulate Particles", "particle_sim.cs")
                .read(render_view.render_view_buffer)
                .write(particle_buffer)
                .write(alive_list)
                .write(alive_list_new)
                .write(dead_list)
                .write(counter_buffer)
                .indirect_size(dispatch_args_buffer);

    render_graph.add_compute_pass("Particle End Update", "particle_end_update.cs")
                .read(counter_buffer)
                .write(instanced_draw_args_buffer)
                .group_size(1, 1, 1)
                .work_size(1, 1, 1);


    const glm::uvec2 render_res = render_view.gpu_view.resolution;
    RasterNode& line_pass = render_graph.add_raster_pass("billboard pass", "billboard.vx", "billboard.px")
                                .topology(Topology::TriangleList)
                                .attribute(AttrFormat::XY32_SFloat)  // Position
                                .attribute(AttrFormat::XY32_SFloat)  // UVs
                                .read(render_view.render_view_buffer, ShaderStages::Vertex)
                                .read(particle_buffer, ShaderStages::Vertex)
                                .read(alive_list, ShaderStages::Vertex)
                                .read(render_view.blue_noise1d->image, ShaderStages::Pixel)
                                .read(point_sampler, ShaderStages::Pixel | ShaderStages::Vertex)
                                .depth_stencil(render_view.dbuffer.image, true, true)
                                .load_op_depth(LoadOp::Load)
                                .load_op_color(LoadOp::Load)
                                .attach(render_image)
                                .raster_extent(render_res.x, render_res.y);
    line_pass.draw_indirect(billboard_vertices, instanced_draw_args_buffer);

    /* clang-format on */
}

void VfxPipeline::deinit(GPUAdapter& gpu) {
    VRAMBank& bank = gpu.get_vram_bank();
    bank.destroy(instanced_draw_args_buffer);
    bank.destroy(dispatch_args_buffer);
    bank.destroy(particle_buffer);
    bank.destroy(dead_list);
    bank.destroy(alive_list_new);
    bank.destroy(alive_list);
    bank.destroy(counter_buffer);
    bank.destroy(emitter_buffer);
    bank.destroy(particle_effects_buffer);
    bank.destroy(billboard_vertices);

    bank.destroy(point_sampler);
}

}  // namespace tmt
