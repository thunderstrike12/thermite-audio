#include "render_view.hpp"

#include <graphite/imgui.hh>
#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>

#include "engine.hpp"
#include "renderer.hpp"
#include "material.hpp"

#include "core/ecs.hpp"
#include "core/window.hpp"
#include "core/logger.hpp"
#include "core/resources.hpp"

#include "core/components/camera.hpp"
#include "core/components/transform.hpp"
#include "engine/tools/player_data.hpp"

namespace tmt {

/* R2 quasi-random sequence. [0, 1) */
static glm::vec2 r2_sequence(const uint32_t n) {
    /* Improved coefficients from <https://www.martysmods.com/a-better-r2-sequence/> */
    /* (1.0 - original coefficients) */
    return glm::fract(glm::vec2((float)n) * glm::vec2(0.2451223337533073f, 0.4301597090019468f));
}

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

    /* clang-format off */
    /* Viewport Texture */
    viewport.texture = bank.create_texture("Viewport Texture", 
        TextureUsage::ColorAttachment | TextureUsage::Sampled | TextureUsage::Storage | TextureUsage::TransferSrc, TextureFormat::RGBA8Unorm, 
        { (uint32_t)engine.window.width, (uint32_t)engine.window.height, 0 }
    ).expect("failed to initialize attachment texture");

    /* Viewport Image */
    viewport.image = bank.create_image("Viewport Image", viewport.texture).expect("failed to initialize attachment image.");

    /* Create the active render view buffer */
    render_view_buffer = bank.create_buffer("Render View Buffer", 
        BufferUsage::Constant | BufferUsage::TransferDst, sizeof(RenderView)
    ).expect("failed to create render view buffer.");

    /* Create the visibility & depth buffer */
    const Size3D full_rate { gpu_view.resolution.x, gpu_view.resolution.y };
    vbuffer.texture = bank.create_texture("Visibility Buffer Texture", TextureUsage::Storage | TextureUsage::Sampled, TextureFormat::RG32Uint, full_rate).expect("failed to create vbuffer texture.");
    vbuffer.image = bank.create_image("Visibility Buffer Image", vbuffer.texture).expect("failed to create vbuffer image.");
    dbuffer.texture =
        bank.create_texture("Depth/Stencil Buffer Texture", TextureUsage::DepthStencil | TextureUsage::Sampled, TextureFormat::D16UnormS8Uint, full_rate).expect("failed to create depth buffer texture.");
    dbuffer.image = bank.create_image("Depth/Stencil Buffer Image", dbuffer.texture).expect("failed to create depth buffer image."); /* Depth Stencil Image View - Used for Writing in Raster Pass */
    prev_dbuffer.texture =
        bank.create_texture("Prev Depth/Stencil Buffer Texture", TextureUsage::DepthStencil | TextureUsage::Sampled, TextureFormat::D16UnormS8Uint, full_rate).expect("failed to create prev depth buffer texture.");
    prev_dbuffer.image = bank.create_image("Prev Depth/Stencil Buffer Image", prev_dbuffer.texture).expect("failed to create prev depth buffer image."); /* Depth Stencil Image View - Used for Writing in Raster Pass */
    
    depth_image = bank.create_image("Depth Buffer Image", dbuffer.texture, true).expect("failed to create depth buffer image."); /* Depth Image View - Used for Reading in Compute Pass */
    prev_depth_image = bank.create_image("Prev Depth Buffer Image", prev_dbuffer.texture, true).expect("failed to create depth buffer image."); /* Depth Image View - Used for Reading in Compute Pass */
    stencil_image = bank.create_image("Stencil Buffer Image", dbuffer.texture, false, true).expect("failed to create stencil buffer image."); /* Stencil Image View - Used for Reading in Compute Pass */
    prev_stencil_image = bank.create_image("Prev Stencil Buffer Image", prev_dbuffer.texture, false, true).expect("failed to create prev stencil buffer image."); /* Stencil Image View - Used for Reading in Compute Pass */

    /* Calculate the specular and diffuse shading resolutions */
    const RendererSettings& settings = engine.player_data.get<RendererSettings>("RendererSettings");
    const Size3D diff_rate = rated_resolution(full_rate, settings.diff_shading_rate);
    const Size3D spec_rate = rated_resolution(full_rate, settings.spec_shading_rate);

    /* Create specular and diffuse intermediate buffers */
    diff_buffer.texture = bank.create_texture("Intermediate Diffuse Buffer Texture", 
        TextureUsage::Storage | TextureUsage::Sampled, TextureFormat::RGBA16Sfloat, diff_rate
    ).expect("failed to create diffuse buffer texture.");
    diff_buffer.image = bank.create_image("Intermediate Diffuse Buffer Image", diff_buffer.texture).expect("failed to create diffuse buffer image.");
    spec_buffer.texture = bank.create_texture("Intermediate Specular Buffer Texture", 
        TextureUsage::Storage | TextureUsage::Sampled, TextureFormat::RGBA16Sfloat, spec_rate
    ).expect("failed to create specular buffer texture.");
    spec_buffer.image = bank.create_image("Intermediate Specular Buffer Image", spec_buffer.texture).expect("failed to create specular buffer image.");

    /* Create the luminance buffer */
    lbuffer.texture = bank.create_texture("Luminance Buffer Texture", TextureUsage::ColorAttachment | TextureUsage::Storage | TextureUsage::Sampled, TextureFormat::RGBA16Sfloat, full_rate)
                          .expect("failed to create lbuffer texture.");
    lbuffer.image = bank.create_image("Luminance Buffer Image", lbuffer.texture).expect("failed to create lbuffer image.");

    /* Create the thresholded luminance buffer */
    tbuffer.meta = { 7, 1 };  // Set 7 mips, 1 array layer
    tbuffer.texture =
        bank.create_texture(
                "Thresholded Luminance Buffer Texture", TextureUsage::ColorAttachment | TextureUsage::Storage | TextureUsage::Sampled, TextureFormat::RGBA16Sfloat, full_rate, tbuffer.meta
        )
            .expect("failed to create tbuffer texture.");
    for (uint32_t curr_mip = 0; curr_mip < tbuffer.meta.mips; curr_mip++)
        tbuffer.images.push_back(bank.create_image("Thresholded Luminance Buffer Image", tbuffer.texture, false, false, curr_mip).expect("failed to create tbuffer image."));

    /* Create the post cg and tonemap buffer (pre chromatic aberration) */
    post_tonemap.texture = bank.create_texture("Post CG/Tonemap Buffer Texture", TextureUsage::Storage | TextureUsage::Sampled, TextureFormat::RGBA8Unorm, full_rate)
                          .expect("failed to create post cg/tonemap texture.");
    post_tonemap.image = bank.create_image("Post CG/Tonemap Buffer Image", post_tonemap.texture).expect("failed to create post cg/tonemap image.");

    /* History Screen Buffers */
    hbuffer1.texture = bank.create_texture("History1 Buffer Texture", 
        TextureUsage::ColorAttachment | TextureUsage::Sampled | TextureUsage::Storage, TextureFormat::RGBA16Sfloat, full_rate
    ).expect("failed to initialize history buffer texture");
    hbuffer1.image = bank.create_image("History1 Buffer Image", hbuffer1.texture).expect("failed to initialize history buffer image.");
    hbuffer2.texture = bank.create_texture("History2 Buffer Texture", 
        TextureUsage::ColorAttachment | TextureUsage::Sampled | TextureUsage::Storage, TextureFormat::RGBA16Sfloat, full_rate
    ).expect("failed to initialize history buffer texture");
    hbuffer2.image = bank.create_image("History2 Buffer Image", hbuffer2.texture).expect("failed to initialize history buffer image.");

    /* Motion Vector Buffer */
    mbuffer.texture = bank.create_texture("Motion Vector Buffer Texture", 
        TextureUsage::ColorAttachment | TextureUsage::Sampled | TextureUsage::Storage, TextureFormat::RG16Sfloat, full_rate
    ).expect("failed to initialize motion vector buffer texture");
    mbuffer.image = bank.create_image("Motion Vector Buffer Image", mbuffer.texture).expect("failed to initialize motion vector buffer image.");

    /* Froxel volume buffers */
    froxel_scatter_texture = bank.create_texture("Froxel Scatter Texture", TextureUsage::Storage | TextureUsage::Sampled, TextureFormat::RGBA16Sfloat, {160, 90, 64}).expect("failed to create froxel texture.");
    froxel_scatter_image = bank.create_image("Froxel Scatter Image", froxel_scatter_texture).expect("failed to create froxel image.");
    prev_froxel_scatter_texture = bank.create_texture("Prev Froxel Scatter Texture", TextureUsage::Storage | TextureUsage::Sampled, TextureFormat::RGBA16Sfloat, {160, 90, 64}).expect("failed to create froxel texture.");
    prev_froxel_scatter_image = bank.create_image("Prev Froxel Scatter Image", prev_froxel_scatter_texture).expect("failed to create froxel image.");
    froxel_luminance_texture = bank.create_texture("Froxel Luminance Texture", TextureUsage::Storage | TextureUsage::Sampled, TextureFormat::RGBA16Sfloat, {160, 90, 64}).expect("failed to create froxel texture.");
    froxel_luminance_image = bank.create_image("Froxel Luminance Image", froxel_luminance_texture).expect("failed to create froxel image.");
    prev_froxel_luminance_texture = bank.create_texture("Prev Froxel Luminance Texture", TextureUsage::Storage | TextureUsage::Sampled, TextureFormat::RGBA16Sfloat, {160, 90, 64}).expect("failed to create froxel texture.");
    prev_froxel_luminance_image = bank.create_image("Prev Froxel Luminance Image", prev_froxel_luminance_texture).expect("failed to create froxel image.");
    froxel_sampler = bank.create_sampler("Froxel Volume Sampler", Filter::Linear, AddressMode::ClampToEdge).expect("failed to create froxel sampler.");

    /* Create the macrofacet cache */
    const uint64_t cache_size = 10'000'000u; /* 480 MB */
    macrofacet_cache = bank.create_buffer("Macrofacet Cache Buffer", BufferUsage::Storage, cache_size, 48ull /* bytes */).expect("failed to create macrofacet cache buffer.");

    /* Load blue noise textures */
    blue_noise2d = engine.resources.load_resource<Texture2D>({ IO::Location::ENGINE, "blue_noise_rg512.png" });
    blue_noise1d = engine.resources.load_resource<Texture2D>({ IO::Location::ENGINE, "blue_noise_r512.png" });
    /* clang-format on */
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
    const RendererSettings& settings = engine.player_data.get<RendererSettings>("RendererSettings");

    /* Calculate camera jitter */
    const glm::vec2 r2 = r2_sequence(frame_counter);  // [0, 1) range
    // offset is in [-0.5, 0.5)
    glm::vec2 pixel_offset = { (r2.x - 0.5f) / (float)gpu_view.resolution.x, (r2.y - 0.5f) / (float)gpu_view.resolution.y };
    // scale to ndc [-1, 1]
    pixel_offset *= 2.0f;

    /* Iterate over all cameras to find an active one to use as render view */
    glm::mat4 p = glm::perspective(glm::radians(camera.fov), aspect_ratio, 0.05f, 1000.0f);
    const glm::mat4 world = transform.get_world_matrix();
    // apply jitter
    if (!engine.renderer.enable_taa) {
        pixel_offset = { 0.0f, 0.0f };
    }
    p[2][0] += pixel_offset.x;
    p[2][1] += pixel_offset.y;
    p[1][1] *= -1.0f;
    gpu_view.world_to_clip = p * glm::inverse(world);  // p * v
    gpu_view.prev_world_to_clip = prev_world_to_clip;
    gpu_view.clip_to_world = glm::inverse(gpu_view.world_to_clip);
    gpu_view.prev_origin = gpu_view.origin;
    gpu_view.origin = glm::vec4(transform.get_world_position(), 0.0f);
    gpu_view.frame_index = frame_counter;
    gpu_view.dt = engine.frame_data().delta_time;
    gpu_view.diff_shading_rate = (uint32_t)settings.diff_shading_rate;
    gpu_view.spec_shading_rate = (uint32_t)settings.spec_shading_rate;
    gpu_view.jitter = pixel_offset;
    gpu_view.prev_jitter = prev_jitter;

    /* Upload the active render view */
    render_graph.upload_buffer(render_view_buffer, &gpu_view, 0u, sizeof(GpuView));
    frame_counter++; /* Update frame counter */

    prev_world_to_clip = gpu_view.world_to_clip;
    prev_jitter = gpu_view.jitter;
}

void RenderView::deinit() {
    VRAMBank& bank = engine.renderer.vram_bank();

    /* Destroy directional albedo LUT */
    // bank.destroy(diralbedo_lut);
    // bank.destroy(diralbedo_lut_texture);
    blue_noise2d = {};
    blue_noise1d = {};

    /* Destroy macrofacet buffers */
    bank.destroy(macrofacet_cache);

    /* Destroy screen buffers */
    bank.destroy(vbuffer.image);
    bank.destroy(vbuffer.texture);
    bank.destroy(dbuffer.image);
    bank.destroy(prev_dbuffer.image);
    bank.destroy(stencil_image);
    bank.destroy(prev_stencil_image);
    bank.destroy(depth_image);
    bank.destroy(prev_depth_image);
    bank.destroy(dbuffer.texture);
    bank.destroy(prev_dbuffer.texture);
    bank.destroy(lbuffer.image);
    bank.destroy(lbuffer.texture);
    bank.destroy(diff_buffer.image);
    bank.destroy(diff_buffer.texture);
    for (auto& timage : tbuffer.images) bank.destroy(timage);
    bank.destroy(tbuffer.texture);
    bank.destroy(post_tonemap.image);
    bank.destroy(post_tonemap.texture);
    bank.destroy(spec_buffer.image);
    bank.destroy(spec_buffer.texture);
    bank.destroy(hbuffer1.image);
    bank.destroy(hbuffer1.texture);
    bank.destroy(hbuffer2.image);
    bank.destroy(hbuffer2.texture);
    bank.destroy(mbuffer.image);
    bank.destroy(mbuffer.texture);
    bank.destroy(viewport.image);
    bank.destroy(viewport.texture);

    bank.destroy(froxel_scatter_image);
    bank.destroy(froxel_scatter_texture);
    bank.destroy(prev_froxel_scatter_image);
    bank.destroy(prev_froxel_scatter_texture);
    bank.destroy(froxel_luminance_image);
    bank.destroy(froxel_luminance_texture);
    bank.destroy(prev_froxel_luminance_image);
    bank.destroy(prev_froxel_luminance_texture);
    bank.destroy(froxel_sampler);

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
    VRAMBank& bank = engine.renderer.vram_bank();
    const Size3D full_rate { gpu_view.resolution.x, gpu_view.resolution.y };

    /* Calculate the specular and diffuse shading resolutions */
    const RendererSettings& settings = engine.player_data.get<RendererSettings>("RendererSettings");
    const Size3D diff_rate = rated_resolution(full_rate, settings.diff_shading_rate);
    const Size3D spec_rate = rated_resolution(full_rate, settings.spec_shading_rate);

    /* Resize the screen buffers */
    bank.resize_texture(viewport.texture, full_rate).expect("failed to resize viewport texture.");
    bank.resize_texture(vbuffer.texture, full_rate).expect("failed to resize vbuffer texture.");
    bank.resize_texture(dbuffer.texture, full_rate).expect("failed to resize depth buffer texture.");
    bank.resize_texture(prev_dbuffer.texture, full_rate).expect("failed to resize depth buffer texture.");
    bank.resize_texture(lbuffer.texture, full_rate).expect("failed to resize lbuffer texture.");
    bank.resize_texture(diff_buffer.texture, diff_rate).expect("failed to resize diffuse buffer texture.");
    bank.resize_texture(spec_buffer.texture, spec_rate).expect("failed to resize specular buffer texture.");
    bank.resize_texture(hbuffer1.texture, full_rate).expect("failed to resize hbuffer texture.");
    bank.resize_texture(hbuffer2.texture, full_rate).expect("failed to resize hbuffer texture.");
    bank.resize_texture(mbuffer.texture, full_rate).expect("failed to resize mbuffer texture.");
    bank.resize_texture(tbuffer.texture, full_rate, tbuffer.meta).expect("failed to resize tbuffer texture.");
    bank.resize_texture(post_tonemap.texture, full_rate).expect("failed to resize post cg/tonemap texture.");
}

}  // namespace tmt
