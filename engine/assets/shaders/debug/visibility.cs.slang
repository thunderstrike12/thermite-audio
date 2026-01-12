import view;
import voxel;
import visibility;
import math;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Object buffers. */
StructuredBuffer<VoxelObject> objects; /* ! Not used ! */
/* Visibility buffer. (Input) */
Texture2D<uint2> vbuffer;
/* Render target. (Output) */
RWTexture2D<float4> render_target;

/* 3D PCG hash function. */
[ForceInline] uint3 pcg3d(const uint3 seed) {
    const uint3 state = seed * 747796405u + 2891336453u;
    const uint3 word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

/* Generate a random color based on a seed. */
[ForceInline] float3 color(const uint seed) {
    const uint3 r = pcg3d(uint3(seed, seed << 12, seed << 24));
    return 0.25 + 0.75 * ((float3)r * (1.0 / float(0xffffffffu)));
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (any(thread_id >= render_view.resolution)) return;

    /* Load visibility buffer data */
    const uint64_t packed = asuint64(vbuffer[thread_id]);

    if (null_visibility(packed)) {
        render_target[thread_id] = float4(0.075f, 0.075f, 0.075f, 1.0f);
    } else {
        /* Unpack visibility buffer data */
        const VisData vis = unpack_visibility(packed);

        /* Render the visibility data as hash */
        render_target[thread_id] = float4(color(voxel_hash(vis.voxel_index, vis.object_index, vis.face_index)), 1.0f);
    }
}
