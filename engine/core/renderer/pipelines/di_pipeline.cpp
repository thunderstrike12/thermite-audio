#include "di_pipeline.hpp"

#include <graphite/render_graph.hh>
#include <graphite/nodes/compute_node.hh>

#include "engine/engine.hpp"
#include "engine/core/renderer/render_view.hpp"
#include "engine/core/renderer/scene_view.hpp"
#include "engine/core/renderer/voxel_object.hpp"
#include "engine/core/renderer/renderer.hpp"

namespace tmt {

/* Divide two numbers, rounding up. */
template <typename T>
T div_up(const T x, const T y) {
    return (x + y - 1) / y;
}

/* clang-format off */

void DiPipeline::enqueue(RenderGraph& render_graph, RenderView& render_view, SceneView& scene_view) {
    /* Get Render Image */
    const BindHandle render_image = render_view.get_render_image();
    const glm::uvec2 render_res = render_view.gpu_view.resolution;
    
    /* Macrofacet reset pass (1 thread per 16 hash cells) */
    render_graph.add_compute_pass("macrofacet reset pass", "macrofacet_reset.cs")
        .read(render_view.render_view_buffer) /* Render view buffer */
        .write(render_view.macrofacet_hashset) /* Macrofacet hash set buffer */
        .write(render_view.macrofacet_shading_commands) /* Shading commands buffer */
        .group_size(128)
        .work_size(render_res.x * render_res.y);

    /* Macrofacet dispatch pass */
    render_graph.add_compute_pass("macrofacet dispatch pass", "macrofacet_dispatch.cs")
        .read(render_view.render_view_buffer) /* Render view buffer */
        .read(render_view.vbuffer.image) /* Visibility buffer */
        .write(render_view.macrofacet_hashset) /* Macrofacet hash set buffer */
        .write(render_view.macrofacet_shading_commands) /* Shading commands buffer */
        .group_size(16, 8)
        .work_size(render_res.x, render_res.y);

    /* Direct illumination pass */
    render_graph.add_compute_pass("direct illumination pass", "direct_illumination.cs")
        .read(render_view.render_view_buffer) /* Render view buffer */
        .read(scene_view.bvh_nodes) /* TLAS nodes buffer */
        .read(scene_view.object_indices) /* Voxel object indices buffer */
        .read(scene_view.object_data) /* Voxel objects buffer */
        .read(render_view.macrofacet_shading_commands) /* Shading commands buffer */
        .write(render_view.macrofacet_illuminance_cache) /* Illuminance cache buffer */
        .group_size(128)
        .indirect_size(render_view.macrofacet_shading_commands);

    /* Debug visualizations */
    if (engine.renderer.display_mode == DisplayMode::ILLUMINANCE) {
        render_graph.add_compute_pass("[debug] illuminance pass", "debug/illuminance.cs")
            .read(render_view.render_view_buffer) /* Render view buffer */
            .read(render_view.vbuffer.image) /* Visibility buffer */
            .read(render_view.macrofacet_illuminance_cache) /* Illuminance cache buffer */
            .write(render_image) /* Render target */
            .group_size(16, 8)
            .work_size(render_res.x, render_res.y);
    }
}

/* clang-format on */

}  // namespace tmt
