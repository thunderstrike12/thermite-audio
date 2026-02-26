#include "ui_pipeline.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>
#include <graphite/nodes/raster_node.hh>

#include <glm/gtx/matrix_decompose.hpp>

#include "engine.hpp"
#include "core/ecs.hpp"
#include "core/logger.hpp"
#include "core/components/ui_component.hpp"
#include "core/components/image_renderer.hpp"

namespace tmt {

/* Vertices that make up a quad */
const float quad_vertices[] {
    /*    pos     */ /*  uv  */
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    1.0f,
    0.0f,
    0.0f,
    1.0f,
    0.0f,
    0.0f,
    1.0f,
    0.0f,
    0.0f,
    1.0f,
    1.0f,
    0.0f,
    0.0f,
    1.0f,
    0.0f,
    1.0f,
    1.0f,
    0.0f,
    1.0f,
    1.0f,
    0.0f,
    1.0f,
    0.0f,
    0.0f,
    1.0f,
};

void UiPipeline::init(GPUAdapter& gpu) {
    VRAMBank& bank = gpu.get_vram_bank();

    /* Create UI Images Buffer */
    images_buffer =
        bank.create_buffer("[UI] Images Buffer", BufferUsage::Storage | BufferUsage::TransferDst, MAX_UI_IMAGES, sizeof(GpuImage)).expect("failed to initialise the ui images buffer.");

    /* Create Image Vertex Buffer */
    image_vertex_buffer =
        bank.create_buffer("[UI] Image Vertex Buffer", BufferUsage::Vertex | BufferUsage::TransferDst, 6u, sizeof(ImageVertex)).expect("failed to initialise the image vertex buffer.");
    bank.upload_buffer(image_vertex_buffer, quad_vertices, 0u, sizeof(quad_vertices));

    /* Create UI image sampler */
    image_sampler = bank.create_sampler("[UI] Image Sampler").expect("failed to create ui image sampler.");
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

    /* Temporary list of image instances */
    std::vector<GpuImage> images {};

    { /* Collect all the images in the scene (Game ECS) */
        const entt::basic_view view = engine.ecs.view<ImageRenderer, UIComponent, Transform>();

        // sort by z order
        std::vector<entt::entity> sorted_entities(view.begin(), view.end());
        std::sort(sorted_entities.begin(), sorted_entities.end(), [&view](entt::entity a, entt::entity b) {
            return view.get<Transform>(a).get_world_position().z < view.get<Transform>(b).get_world_position().z;
        });

        for (const auto entity : sorted_entities) {
            /* Get the components for this entity */
            auto [image_renderer, ui_component, transform] = view.get(entity);
            /* Skip disabled transforms */
            // if (transform.) == false) continue;

            /* Get the world matrix for this image instance */
            glm::mat4 world = transform.get_world_matrix();

            /* Fill out all the image instance attributes */
            GpuImage& image = images.emplace_back();
            decompose_matrix(world, image.pos, image.extent, image.angles);
            image.extent = image.extent * ui_component.size;
            image.pivot = ui_component.pivot;
            image.color = image_renderer.color;
            if (image_renderer.texture) image.image_index = image_renderer.texture.resource->image.get_index();

            glm::vec2 anchor_offset = AnchorHelper::calculate_anchor_offset(entity);
            image.pos += anchor_offset;
        }
    }

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

void UiPipeline::deinit(GPUAdapter& gpu) {
    VRAMBank& bank = gpu.get_vram_bank();

    bank.destroy(images_buffer);
    bank.destroy(image_vertex_buffer);
    bank.destroy(image_sampler);
}

}  // namespace tmt