import view;
import visibility;
import voxel;
import math;

static const int OUTLINE_RADIUS = 2;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Object buffers. */
StructuredBuffer<VoxelObject> objects;
/* Visibility buffer. */
Texture2D<uint2> vbuffer;
/* Render target. */
RWTexture2D<float4> render_target;

/* Check a neighbouring pixel for the object outline flag. */
[ForceInline] bool check_neighbour(const int2 pixel, const int2 offset) {
    /* Bounds check */
    const int2 neighbour = pixel + offset;
    if (any(neighbour >= render_view.resolution || neighbour < 0)) return false;

    /* Load the pixel object data and check if it has the outline flag enabled */
    const uint vy = vbuffer[neighbour].y;
    return vy != uint::maxValue && (objects[vy & 0xFFFFu].object_flags & 0b1u) == 0b1u;
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */ 
    if (any(thread_id >= render_view.resolution)) return;

    /* Only process pixels which do not have the outline flag */
    const uint vy = vbuffer[thread_id].y;
    if (vy != uint::maxValue && (objects[vy & 0xFFFFu].object_flags & 0b1u) == 0b1u) return;

    /* Check neighbouring pixels to see if this pixel should be an outline */
    bool is_outline = false;
    is_outline |= check_neighbour(thread_id, int2( 0,  OUTLINE_RADIUS));
    is_outline |= check_neighbour(thread_id, int2( OUTLINE_RADIUS,  0));
    is_outline |= check_neighbour(thread_id, int2( 0, -OUTLINE_RADIUS));
    is_outline |= check_neighbour(thread_id, int2(-OUTLINE_RADIUS, 0));
    if (is_outline == false) return;

    /* Write outline color */
    render_target[thread_id] = float4(0.95686274f, 0.60392156f, 0.21960784f, 1.0f);
}
