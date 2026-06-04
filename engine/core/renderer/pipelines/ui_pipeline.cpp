#include "ui_pipeline.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>
#include <graphite/nodes/raster_node.hh>

#include <glm/gtx/matrix_decompose.hpp>

#include "engine.hpp"
#include "engine/systems/ui/text_layout.hpp"

#include "core/ecs.hpp"
#include "core/logger.hpp"
#include "core/resources/font.hpp"
#include "core/components/ui_component.hpp"
#include "core/components/image_renderer.hpp"
#include "core/components/text_renderer.hpp"

namespace tmt {

/* clang-format off */
/* Vertices that make up a quad. */
const float quad_vertices[] {
    /*    pos     */ /*  uv  */
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
};
/* clang-format on */

struct ImageVertex {
    glm::vec3 pos {};
    glm::vec2 uvs {};
};

/* GPU data for a single image instance. */
struct GpuImage {
    glm::vec2 pos {};
    glm::vec2 extent {};
    glm::vec4 color {};
    glm::vec3 angles {};
    uint32_t image_index {};
    glm::vec2 pivot {};
    uint32_t flipbook_frame {};
    uint32_t flipbook_frames {};
};

/* GPU data for a single 3d image instance. */
struct GpuImage3D {
    glm::mat4 local_to_world {};
    glm::vec4 color {};
    glm::vec2 pivot {};
    uint32_t flipbook_frame {};
    uint32_t flipbook_frames {};
    uint32_t image_index {};
    uint32_t p0 {}, p1 {}, p2 {}; /* padding */
};

/* GPU data for a single text glyph (char) instance. */
struct GpuGlyph {
    glm::vec4 pos_size {};   /* .xy = screen position, .zw = glyph size */
    glm::vec4 uv_rect {};    /* .xy = min UV, .zw = max UV */
    glm::vec4 color {};      /* Text color (RGBA Rec.709) */
    glm::vec4 glow_color {}; /* Glow color (RGBA Rec.709) | .rgb = color, .a = strength */
    glm::vec4 params {};     /* .x = atlas_index, .y = glow_radius_px, .z = glow_boost, .w = unused */
    glm::vec3 angles {};
    float pad {};
};

/* GPU data for a single text glyph (char) instance. */
struct GpuGlyph3D {
    glm::vec4 pos_size {}; /* .xy = screen position, .zw = glyph size */
    glm::vec4 uv_rect {};  /* .xy = min UV, .zw = max UV */
    glm::vec4 color {};    /* Text color (RGBA Rec.709) */
    glm::vec4 params {};   /* .x = atlas_index, .yzw = unused */
    glm::mat4 local_to_world {};
};

void UiPipeline::init(GPUAdapter& gpu) {
    VRAMBank& bank = gpu.get_vram_bank();

    /* Create UI Images Buffer */
    images_buffer =
        bank.create_buffer("[UI] Images Buffer", BufferUsage::Storage | BufferUsage::TransferDst, MAX_UI_IMAGES, sizeof(GpuImage)).expect("failed to initialise the ui images buffer.");
    images_3d_buffer =
        bank.create_buffer("[UI] 3D Images Buffer", BufferUsage::Storage | BufferUsage::TransferDst, MAX_UI_IMAGES, sizeof(GpuImage3D)).expect("failed to initialise the ui images buffer.");

    /* Create Image Vertex Buffer */
    image_vertex_buffer =
        bank.create_buffer("[UI] Image Vertex Buffer", BufferUsage::Vertex | BufferUsage::TransferDst, 6u, sizeof(ImageVertex)).expect("failed to initialise the image vertex buffer.");
    bank.upload_buffer(image_vertex_buffer, quad_vertices, 0u, sizeof(quad_vertices));

    /* Create UI image sampler */
    image_sampler = bank.create_sampler("[UI] Image Sampler").expect("failed to create ui image sampler.");

    /* Create Text Glyphs Buffer */
    glyphs_buffer =
        bank.create_buffer("[UI] Glyphs Buffer", BufferUsage::Storage | BufferUsage::TransferDst, MAX_UI_GLYPHS, sizeof(GpuGlyph)).expect("failed to initialise the glyphs buffer.");
    glyphs_3d_buffer =
        bank.create_buffer("[UI] 3D Glyphs Buffer", BufferUsage::Storage | BufferUsage::TransferDst, MAX_UI_GLYPHS, sizeof(GpuGlyph3D)).expect("failed to initialise the glyphs buffer.");

    /* Create Glyph Vertex Buffer (shares quad vertices with images) */
    glyph_vertex_buffer =
        bank.create_buffer("[UI] Glyph Vertex Buffer", BufferUsage::Vertex | BufferUsage::TransferDst, 6u, sizeof(ImageVertex)).expect("failed to initialise the glyph vertex buffer.");
    bank.upload_buffer(glyph_vertex_buffer, quad_vertices, 0u, sizeof(quad_vertices));

    /* Create Text sampler with linear filtering for SDF */
    text_sampler = bank.create_sampler("[UI] Text Sampler", Filter::Linear, AddressMode::ClampToEdge).expect("failed to create text sampler.");

    /* Initialize font manager for default font */
    FontManager::init();
}

/* Decompose a transform world matrix into the attribute we need for image instances. */
static inline void decompose_matrix(const glm::mat4& m, glm::vec2& pos, glm::vec2& extent, glm::vec3& angles) {
    glm::vec3 scale {};
    glm::vec3 translation {};
    glm::vec3 skew {};
    glm::vec4 perspective {};
    glm::quat orientation {};

    // Decompose the matrix into its components.
    glm::decompose(m, scale, orientation, translation, skew, perspective);

    // Fill our attributes:
    // - Use the X and Y components for 2D position and extent.
    pos = glm::vec2(translation.x, translation.y);
    extent = glm::vec2(scale.x, scale.y);

    // Convert the quaternion to Euler angles (in radians).
    angles = glm::eulerAngles(orientation);
}

void UiPipeline::enqueue(RenderGraph& render_graph, RenderView& render_view) {
    if (!render_ui_pipeline) return;

    enqueue_images(render_graph, render_view);

    /* Render text on top */
    if (render_text) {
        enqueue_text(render_graph, render_view);
    }
}

void UiPipeline::enqueue_images(RenderGraph& render_graph, RenderView& render_view) {
    /* Temporary list of image instances */
    std::vector<GpuImage> images {};
    std::vector<GpuImage3D> images_3d {};

    /* Factor to convert authored (1920x1080) positions to actual screen pixels. */
    const glm::vec2 pos_scale_factor(
        static_cast<float>(render_view.gpu_view.resolution.x) / UIComponent::REFERENCE_WIDTH, static_cast<float>(render_view.gpu_view.resolution.y) / UIComponent::REFERENCE_HEIGHT
    );

    { /* Collect all the images in the scene (Game ECS) */
        const entt::basic_view view = engine.ecs.view<ImageRenderer, UIComponent, Transform>();

        // sort by z order
        std::vector<entt::entity> sorted_entities(view.begin(), view.end());
        std::sort(sorted_entities.begin(), sorted_entities.end(), [&view](entt::entity a, entt::entity b) {
            return view.get<Transform>(a).get_world_position().z < view.get<Transform>(b).get_world_position().z;
        });

        for (const auto entity : sorted_entities) {
            /* Get the components for this entity */
            auto&& [image_renderer, ui_component, transform] = view.get(entity);
            /* Skip disabled transforms */
            // if (transform.) == false) continue;

            /* flipbook frame animation */
            if (image_renderer.texture && !image_renderer.finished) {
                const uint32_t frame_count = image_renderer.texture.resource->flipbook_frames;
                if (frame_count <= 1) {
                    image_renderer.current_frame = 0;
                } else {
                    image_renderer.accumulated_time += engine.frame_data().delta_time * image_renderer.anim_speed;
                    const uint32_t total_frames_elapsed = uint32_t(image_renderer.accumulated_time);

                    if (image_renderer.continuous_anim) {
                        image_renderer.current_frame = total_frames_elapsed % frame_count;
                    } else {
                        const uint32_t loops = total_frames_elapsed / frame_count;
                        if (loops >= image_renderer.loop_count) {
                            image_renderer.current_frame = frame_count - 1;
                            image_renderer.completed_loops = image_renderer.loop_count;
                            image_renderer.finished = true;
                        } else {
                            image_renderer.current_frame = total_frames_elapsed % frame_count;
                            image_renderer.completed_loops = loops;
                        }
                    }
                }
            }

            /* Get the world matrix for this image instance */
            glm::mat4 world = transform.get_world_matrix();

            /* Fill the parameters of the gpu image */
            GpuImage& image = images.emplace_back();
            decompose_matrix(world, image.pos, image.extent, image.angles);
            /* Scale authored position to real screen pixels before anchor is applied. */
            image.pos *= pos_scale_factor;
            image.extent = image.extent * ui_component.real_size;
            image.pivot = ui_component.pivot;
            image.color = image_renderer.color;
            if (image_renderer.texture) image.image_index = image_renderer.texture.resource->image.get_index();
            image.flipbook_frame = image_renderer.current_frame;
            image.flipbook_frames = image_renderer.texture ? image_renderer.texture.resource->flipbook_frames : 1u;
            image.pos += AnchorHelper::calculate_anchor_offset(entity);
        }
    }

    if (!images.empty()) {
        /* Upload the image instances */
        render_graph.upload_buffer(images_buffer, images.data(), 0u, images.size() * sizeof(GpuImage));
        image_count = (uint32_t)images.size();

        /* Get Render Image */
        const BindHandle render_image = render_view.get_render_image();

        /* Get Render Resolution */
        const glm::uvec2 render_res = render_view.gpu_view.resolution;

        /* UI overlay rendering */
        /* clang-format off */
        RasterNode& image_pass = render_graph.add_raster_pass("ui images", "ui/ui_image.vx", "ui/ui_image.px")
            /* Vertex stage */
            .topology(Topology::TriangleList)
            .attribute(AttrFormat::XYZ32_SFloat) /* Position */
            .attribute(AttrFormat::XY32_SFloat)  /* UV */
            .read(render_view.render_view_buffer, ShaderStages::Vertex)
            /* Pixel stage */
            .read(images_buffer, ShaderStages::Vertex | ShaderStages::Pixel)
            .read(image_sampler, ShaderStages::Pixel)
            .alpha_blending(true)
            .attach(render_image)
            .raster_extent(render_res.x, render_res.y);
        /* clang-format on */

        /* Draw all images with 1 draw call, using instancing & bindless textures. */
        image_pass.draw(image_vertex_buffer, 6u, 0u, image_count);
    }

    { /* Collect all the images in the scene (Game ECS) */
        const entt::basic_view view = engine.ecs.view<ImageRenderer, Transform>(entt::exclude<UIComponent>);

        for (auto&& [entity, image_renderer, transform] : view.each()) {
            /* Fill the parameters of the gpu image */
            GpuImage3D& image = images_3d.emplace_back();
            image.color = image_renderer.color;
            if (image_renderer.texture) image.image_index = image_renderer.texture.resource->image.get_index();
            image.flipbook_frame = image_renderer.current_frame;
            image.flipbook_frames = image_renderer.texture ? image_renderer.texture.resource->flipbook_frames : 1u;
            image.local_to_world = transform.get_world_matrix();
        }
    }

    if (!images_3d.empty()) {
        /* Upload the image instances */
        render_graph.upload_buffer(images_3d_buffer, images_3d.data(), 0u, images_3d.size() * sizeof(GpuImage3D));
        image_count += (uint32_t)images_3d.size();

        /* Get Render Image */
        const BindHandle render_image = render_view.get_render_image();

        /* Get Render Resolution */
        const glm::uvec2 render_res = render_view.gpu_view.resolution;

        /* UI overlay rendering */
        /* clang-format off */
        const bool flip = (render_view.frame_counter & 0b1u) == 0u;
        RasterNode& image_pass = render_graph.add_raster_pass("ui images 3d", "ui/3d_image.vx", "ui/3d_image.px")
            /* Vertex stage */
            .topology(Topology::TriangleList)
            .attribute(AttrFormat::XYZ32_SFloat) /* Position */
            .attribute(AttrFormat::XY32_SFloat)  /* UV */
            .read(render_view.render_view_buffer, ShaderStages::Vertex)
            /* Pixel stage */
            .read(images_3d_buffer, ShaderStages::Vertex | ShaderStages::Pixel)
            .read(image_sampler, ShaderStages::Pixel)
            .depth_stencil(flip ? render_view.dbuffer.image : render_view.prev_dbuffer.image, true, true)
            .load_op_depth(LoadOp::Load)
            .load_op_color(LoadOp::Load)
            .alpha_blending(true)
            .attach(render_image)
            .raster_extent(render_res.x, render_res.y);
        /* clang-format on */

        /* Draw all images with 1 draw call, using instancing & bindless textures. */
        image_pass.draw(image_vertex_buffer, 6u, 0u, (uint32_t)images_3d.size());
    }
}

void UiPipeline::enqueue_text(RenderGraph& render_graph, RenderView& render_view) {
    std::vector<GpuGlyph> glyphs {};
    std::vector<GpuGlyph3D> glyphs_3d {};

    /* Factor to convert authored (1920x1080) positions to actual screen pixels. */
    const glm::vec2 pos_scale_factor(
        static_cast<float>(render_view.gpu_view.resolution.x) / UIComponent::REFERENCE_WIDTH, static_cast<float>(render_view.gpu_view.resolution.y) / UIComponent::REFERENCE_HEIGHT
    );

    { /* Collect all the text in the scene (Game ECS) */
        const entt::basic_view view = engine.ecs.view<TextRenderer, UIComponent, Transform>();

        /* Z ordering */
        std::vector<entt::entity> sorted_entities(view.begin(), view.end());
        std::sort(sorted_entities.begin(), sorted_entities.end(), [&view](entt::entity a, entt::entity b) {
            return view.get<Transform>(a).get_world_position().z < view.get<Transform>(b).get_world_position().z;
        });

        for (const auto entity : sorted_entities) {
            /* Get the components for this entity */
            auto& text_renderer = engine.ecs.get_component<TextRenderer>(entity);
            const auto& ui_component = engine.ecs.get_component<UIComponent>(entity);
            const auto& transform = engine.ecs.get_component<Transform>(entity);

            /* Skip empty text */
            if (text_renderer.text.empty()) continue;

            /* Build a scaled copy of the renderer for layout.
               All authored pixel values (font_size, letter_spacing, max_width, max_height)
               are in 1920x1080 space and must be converted to real screen pixels.
               We never write back to the component — this copy is layout-only. */
            TextRenderer layout_tr = text_renderer;
            layout_tr.font_size = text_renderer.font_size * pos_scale_factor.y;
            layout_tr.letter_spacing = text_renderer.letter_spacing * pos_scale_factor.y;
            if (text_renderer.use_ui_component_size) {
                layout_tr.max_width = ui_component.real_size.x;
                layout_tr.max_height = ui_component.real_size.y;
            } else {
                if (text_renderer.max_width > 0) layout_tr.max_width = text_renderer.max_width * pos_scale_factor.x;
                if (text_renderer.max_height > 0) layout_tr.max_height = text_renderer.max_height * pos_scale_factor.y;
            }

            /* Get the font (use default if none specified) */
            const Font* font = nullptr;
            if (text_renderer.font && text_renderer.font.resource) {
                font = text_renderer.font.resource.get();
            }
            if (!font || !font->is_valid()) {
                font = FontManager::get_default_font().get();
            }
            if (!font || !font->is_valid()) {
                continue; /* No font available */
            }

            /* Calculate the base position from transform, scaled to actual screen pixels. */
            glm::vec2 base_pos = glm::vec2(transform.get_world_position()) * pos_scale_factor;
            glm::vec2 anchor_offset = AnchorHelper::calculate_anchor_offset(entity);
            base_pos += anchor_offset;

            /* Apply pivot offset */
            glm::vec2 pivot_offset = ui_component.pivot * ui_component.real_size;
            base_pos -= pivot_offset;

            /* Calculate container size for text layout */
            glm::vec2 container_size = ui_component.real_size;

            /* Perform text layout using the scaled copy */
            TextLayoutResult layout = TextLayout::layout(layout_tr, font, container_size);

            glm::vec3 scale {};
            glm::vec3 translation {};
            glm::vec3 skew {};
            glm::vec4 perspective {};
            glm::quat orientation {};

            // Decompose the matrix into its components.
            glm::decompose(transform.get_world_matrix(), scale, orientation, translation, skew, perspective);

            const glm::vec3 angles = glm::eulerAngles(orientation);

            /* Compute the rotation center (pivot point of the text block) */
            const glm::vec2 rotation_center = base_pos + ui_component.pivot * container_size;

            /* Build the rotation matrix from the decomposed quaternion */
            const glm::mat3 rot = glm::mat3_cast(orientation);

            /* Convert layout glyphs to GPU format */
            for (const auto& glyph : layout.glyphs) {
                if (glyphs.size() >= MAX_UI_GLYPHS) {
                    Log::warn(Log::Scope::RENDERER, "Max UI glyphs reached ({})", MAX_UI_GLYPHS);
                    break;
                }

                /* Glyph position before rotation (in the XY plane, Z = 0) */
                glm::vec2 glyph_pos_2d = base_pos + glyph.position;
                const glm::vec3 offset = glm::vec3(glyph_pos_2d - rotation_center, 0.0f) * glm::vec3(scale.x, scale.y, 1.0f);

                /* Rotate the offset with the full 3D rotation */
                const glm::vec3 rotated_offset = rot * offset;

                /* Project back to 2D */
                glyph_pos_2d = rotation_center + glm::vec2(rotated_offset);

                /* Fill in the GPU glyph struct */
                GpuGlyph& gpu_glyph = glyphs.emplace_back();
                gpu_glyph.pos_size.x = glyph_pos_2d.x;
                gpu_glyph.pos_size.y = glyph_pos_2d.y;
                gpu_glyph.pos_size.z = glyph.size.x * scale.x;
                gpu_glyph.pos_size.w = glyph.size.y * scale.y;
                gpu_glyph.uv_rect = glyph.uv_rect;
                gpu_glyph.color = glyph.color;
                gpu_glyph.glow_color = text_renderer.glow_color;
                gpu_glyph.params.x = *reinterpret_cast<const float*>(&glyph.atlas_index);
                gpu_glyph.params.y = text_renderer.glow_radius_px;
                gpu_glyph.params.z = text_renderer.glow_boost;
                gpu_glyph.angles = angles;
            }
        }
    }

    if (!glyphs.empty()) {
        /* Upload the glyph instances */
        render_graph.upload_buffer(glyphs_buffer, glyphs.data(), 0u, glyphs.size() * sizeof(GpuGlyph));
        glyph_count = (uint32_t)glyphs.size();

        /* Get Render Image */
        const BindHandle render_image = render_view.get_render_image();
        const glm::uvec2 render_res = render_view.gpu_view.resolution;

        /* Text rendering pass */
        /* clang-format off */
        RasterNode& text_pass = render_graph.add_raster_pass("ui text", "ui/ui_text.vx", "ui/ui_text.px")
            /* Vertex stage */
            .topology(Topology::TriangleList)
            .attribute(AttrFormat::XYZ32_SFloat) /* Position */
            .attribute(AttrFormat::XY32_SFloat)  /* UV */
            .read(render_view.render_view_buffer, ShaderStages::Vertex)
            /* Pixel stage */
            .read(glyphs_buffer, ShaderStages::Vertex | ShaderStages::Pixel)
            .read(text_sampler, ShaderStages::Pixel)
            .alpha_blending(true)
            .attach(render_image)
            .raster_extent(render_res.x, render_res.y);
        /* clang-format on */

        /* Draw all glyphs with 1 draw call, using instancing & bindless textures. */
        text_pass.draw(glyph_vertex_buffer, 6u, 0u, glyph_count);
    }

    { /* Collect all the text in the scene (Game ECS) */
        const entt::basic_view view = engine.ecs.view<TextRenderer, Transform>(entt::exclude<UIComponent>);

        for (auto&& [entity, text_renderer, transform] : view.each()) {
            /* Skip empty text */
            if (text_renderer.text.empty()) continue;

            /* Get the font (use default if none specified) */
            const Font* font = nullptr;
            if (text_renderer.font && text_renderer.font.resource) {
                font = text_renderer.font.resource.get();
            }
            if (!font || !font->is_valid()) {
                font = FontManager::get_default_font().get();
            }
            if (!font || !font->is_valid()) {
                continue; /* No font available */
            }

            /* Perform text layout */
            TextLayoutResult layout = TextLayout::layout(text_renderer, font, glm::vec2(transform.get_world_scale()));

            /* Convert layout glyphs to GPU format */
            for (const auto& glyph : layout.glyphs) {
                if (glyphs_3d.size() >= MAX_UI_GLYPHS) {
                    Log::warn(Log::Scope::RENDERER, "Max UI glyphs reached ({})", MAX_UI_GLYPHS);
                    break;
                }

                /* Fill in the GPU glyph struct */
                GpuGlyph3D& gpu_glyph = glyphs_3d.emplace_back();
                gpu_glyph.pos_size.x = glyph.position.x;
                gpu_glyph.pos_size.y = glyph.position.y;
                gpu_glyph.pos_size.z = glyph.size.x;
                gpu_glyph.pos_size.w = glyph.size.y;
                gpu_glyph.uv_rect = glyph.uv_rect;
                gpu_glyph.color = glyph.color;
                gpu_glyph.params.x = *reinterpret_cast<const float*>(&glyph.atlas_index);
                gpu_glyph.local_to_world = transform.get_world_matrix();
            }
        }
    }

    if (!glyphs_3d.empty()) {
        /* Upload the glyph instances */
        render_graph.upload_buffer(glyphs_3d_buffer, glyphs_3d.data(), 0u, glyphs_3d.size() * sizeof(GpuGlyph3D));
        glyph_count += (uint32_t)glyphs_3d.size();

        /* Get Render Image */
        const BindHandle render_image = render_view.get_render_image();
        const glm::uvec2 render_res = render_view.gpu_view.resolution;

        /* Text rendering pass */
        /* clang-format off */
        const bool flip = (render_view.frame_counter & 0b1u) == 0u;
        RasterNode& text_pass = render_graph.add_raster_pass("ui text 3d", "ui/3d_text.vx", "ui/3d_text.px")
            /* Vertex stage */
            .topology(Topology::TriangleList)
            .attribute(AttrFormat::XYZ32_SFloat) /* Position */
            .attribute(AttrFormat::XY32_SFloat)  /* UV */
            .read(render_view.render_view_buffer, ShaderStages::Vertex)
            /* Pixel stage */
            .read(glyphs_3d_buffer, ShaderStages::Vertex | ShaderStages::Pixel)
            .read(text_sampler, ShaderStages::Pixel)
            .depth_stencil(flip ? render_view.dbuffer.image : render_view.prev_dbuffer.image, true, true)
            .load_op_depth(LoadOp::Load)
            .load_op_color(LoadOp::Load)
            .alpha_blending(true)
            .attach(render_image)
            .raster_extent(render_res.x, render_res.y);
        /* clang-format on */

        /* Draw all glyphs with 1 draw call, using instancing & bindless textures. */
        text_pass.draw(glyph_vertex_buffer, 6u, 0u, (uint32_t)glyphs_3d.size());
    }
}

void UiPipeline::deinit(GPUAdapter& gpu) {
    VRAMBank& bank = gpu.get_vram_bank();

    bank.destroy(images_buffer);
    bank.destroy(images_3d_buffer);
    bank.destroy(image_vertex_buffer);
    bank.destroy(image_sampler);

    bank.destroy(glyphs_buffer);
    bank.destroy(glyphs_3d_buffer);
    bank.destroy(glyph_vertex_buffer);
    bank.destroy(text_sampler);
}

}  // namespace tmt