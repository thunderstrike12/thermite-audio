#include "render_view.hpp"

#include <graphite/imgui.hh>
#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>

#include "engine.hpp"
#include "renderer.hpp"

#include "core/ecs.hpp"
#include "core/window.hpp"
#include "core/logger.hpp"

#include "core/components/camera.hpp"
#include "core/components/transform.hpp"

namespace tmt {

void RenderView::init() {
    VRAMBank& bank = engine.renderer.vram_bank();

    /* Initialize the Render Target */
    const TargetDesc target { static_cast<HWND>(engine.window.get_window_handle()) };
    if (const Result r = bank.create_render_target(target); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialize render target.\nreason: {}", r.unwrap_err());
        return;
    } else {
        render_target = r.unwrap();
    }

    /* Viewport Texture */
    viewport.texture = bank.create_texture(
                               "Viewport Texture", TextureUsage::ColorAttachment | TextureUsage::Sampled | TextureUsage::Storage, TextureFormat::RGBA8Unorm,
                               { (uint32_t)engine.window.width, (uint32_t)engine.window.height, 0 }, { 1, 1 }
    )
                           .expect("failed to initialize attachment texture");

    /* Viewport Image */
    viewport.image = bank.create_image("Viewport Image", viewport.texture).expect("failed to initialize attachment image.");

    /* Create the active render view buffer */
    render_view_buffer = bank.create_buffer("Render View Buffer", BufferUsage::Constant | BufferUsage::TransferDst, sizeof(RenderView)).expect("failed to create render view buffer.");

    /* Create the visibility buffer */
    const Size3D view_size { gpu_view.resolution.x, gpu_view.resolution.y };
    vbuffer.texture =
        bank.create_texture("Visibility Buffer Texture", TextureUsage::Storage | TextureUsage::Sampled, TextureFormat::RG32Uint, view_size).expect("failed to create vbuffer texture.");
    vbuffer.image = bank.create_image("Visibility Buffer Image", vbuffer.texture).expect("failed to create vbuffer image.");
    dbuffer.texture = bank.create_texture("Depth Buffer Texture", TextureUsage::DepthStencil, TextureFormat::D32Sfloat, view_size).expect("failed to create depth buffer texture.");
    dbuffer.image = bank.create_image("Depth Buffer Image", dbuffer.texture).expect("failed to create depth buffer image.");

    /* Create the illuminance buffer */
    ibuffer.texture = bank.create_texture("Illuminance Buffer Texture", TextureUsage::Storage, TextureFormat::RG11B10Ufloat, view_size).expect("failed to create ibuffer texture.");
    ibuffer.image = bank.create_image("Illuminance Buffer Image", vbuffer.texture).expect("failed to create ibuffer image.");

    /* Create the macrofacet buffers */
    const uint64_t hashkey_size = sizeof(uint64_t);
    const uint64_t hashset_size = (uint64_t)view_size.x * view_size.y;
    macrofacet_hashset = bank.create_buffer("Macrofacet Hashset Buffer", BufferUsage::Storage, hashset_size, hashkey_size).expect("failed to create macrofacet hashset buffer.");
    macrofacet_shading_commands = bank.create_buffer("Macrofacet Shading Commands Buffer", BufferUsage::Storage | BufferUsage::Indirect, hashset_size, hashkey_size)
                                      .expect("failed to create macrofacet shading commands buffer.");
    const uint64_t cache_element_size = hashkey_size + sizeof(uint64_t) * 2ull;
    const uint64_t cache_size = 10'000'000u;
    macrofacet_illuminance_cache =
        bank.create_buffer("Macrofacet Illuminance Cache Buffer", BufferUsage::Storage, cache_size, cache_element_size).expect("failed to create macrofacet illuminance cache buffer.");
}

void RenderView::update() {
#ifndef THERMITE_EDITOR
    gpu_view.resolution = glm::uvec2(engine.window.width, engine.window.height);
#endif  // !THERMITE_EDITOR

    /* Resize the render target if the window was resized */
    if (engine.window.resized) {
        VRAMBank& bank = engine.renderer.vram_bank();

        /* Resize the render target */
        if (const Result r = bank.resize_render_target(render_target, gpu_view.resolution.x, gpu_view.resolution.y); r.is_err()) {
            Log::error(Log::Scope::RENDERER, "failed to resize the swapchain.\nreason: {}", r.unwrap_err().c_str());
        }

#ifndef THERMITE_EDITOR
        resize_textures();
#endif  // !THERMITE_EDITOR

        engine.window.resized = false;
    }
}

void RenderView::update_gpu_view(RenderGraph& render_graph, const Camera& camera, const Transform& transform) {
    const float aspect_ratio = (float)gpu_view.resolution.x / (float)gpu_view.resolution.y;

    /* Iterate over all cameras to find an active one to use as render view */
    glm::mat4 p = glm::perspective(glm::radians(camera.fov), aspect_ratio, 0.05f, 1000.0f);
    const glm::mat4 world = transform.get_world_matrix();
    p[1][1] *= -1.0f;
    gpu_view.world_to_clip = p * glm::inverse(world);  // p * v
    gpu_view.clip_to_world = glm::inverse(gpu_view.world_to_clip);
    gpu_view.origin = glm::vec4(transform.get_world_position(), 0.0f);
    gpu_view.frame_index = frame_counter;
    gpu_view.dt = engine.frame_data().delta_time;

    /* Upload the active render view */
    render_graph.upload_buffer(render_view_buffer, &gpu_view, 0u, sizeof(GpuView));
    frame_counter++; /* Update frame counter */
}

void RenderView::deinit() {
    VRAMBank& bank = engine.renderer.vram_bank();

    /* Destroy macrofacet buffers */
    bank.destroy(macrofacet_hashset);
    bank.destroy(macrofacet_shading_commands);
    bank.destroy(macrofacet_illuminance_cache);

    /* Destroy screen buffers */
    bank.destroy(vbuffer.image);
    bank.destroy(vbuffer.texture);
    bank.destroy(dbuffer.image);
    bank.destroy(dbuffer.texture);
    bank.destroy(ibuffer.image);
    bank.destroy(ibuffer.texture);
    bank.destroy(viewport.texture);
    bank.destroy(viewport.image);

    bank.destroy(render_view_buffer);
    bank.destroy(render_target);
}

BindHandle RenderView::get_render_image() const {
#ifdef THERMITE_EDITOR
    return viewport.image;
#else
    return render_target;
#endif  // THERMITE_EDITOR
}

void RenderView::set_viewport_size(uint32_t width, uint32_t height) {
    if (width != gpu_view.resolution.x || height != gpu_view.resolution.y) {
        height = height <= 0 ? 1 : height;
        gpu_view.resolution = { width, height };
        resize_textures();

#ifdef THERMITE_EDITOR
        engine.renderer.imgui->remove_image(viewport.image);
        imgui_viewport = engine.renderer.imgui->add_image(viewport.image);
#endif
    }
}

/* Convert a pixel coordinate to a normalized device coordinate. */
inline glm::vec2 pixel_to_ndc(glm::ivec2 pixel, glm::uvec2 resolution) {
    return ((glm::vec2(pixel) + 0.5f) / glm::vec2(resolution)) * 2.0f - 1.0f;
}

Ray RenderView::pixel_ray(glm::ivec2 pixel) const {
    /* Convert the pixel coordinate to normalized device coordinate */
    const glm::vec2 ndc = pixel_to_ndc(pixel, gpu_view.resolution);

    /* Find the world-space position and convert it to a world-space direction */
    glm::vec4 world_pos = gpu_view.clip_to_world * glm::vec4(ndc, 1.0f, 1.0f);
    world_pos = glm::vec4(glm::vec3(world_pos) / world_pos.w, world_pos.w);
    return Ray(glm::vec3(gpu_view.origin), glm::normalize(glm::vec3(world_pos) - glm::vec3(gpu_view.origin)));
}

void RenderView::resize_textures() {
    const Size3D view_size { gpu_view.resolution.x, gpu_view.resolution.y };
    VRAMBank& bank = engine.renderer.vram_bank();

    /* Resize the screen buffers */
    bank.resize_texture(viewport.texture, view_size).expect("failed to resize viewport texture.");
    bank.resize_texture(vbuffer.texture, view_size).expect("failed to resize vbuffer texture.");
    bank.resize_texture(dbuffer.texture, view_size).expect("failed to resize depth buffer texture.");
    bank.resize_texture(ibuffer.texture, view_size).expect("failed to resize ibuffer texture.");

    /* Resize macrofacet buffers */
    const uint64_t hashkey_size = sizeof(uint64_t);
    const uint64_t hashset_size = (uint64_t)view_size.x * view_size.y;
    bank.resize_buffer(macrofacet_hashset, hashset_size, hashkey_size).expect("failed to resize macrofacet hashset buffer");
    bank.resize_buffer(macrofacet_shading_commands, hashset_size, hashkey_size).expect("failed to resize macrofacet shading commands buffer");
}

}  // namespace tmt
