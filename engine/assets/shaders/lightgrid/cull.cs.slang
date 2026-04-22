import lights;
import math;
import lightgrid;

StructuredBuffer<uint32_t> cascades_bitmasks;

StructuredBuffer<Light> lights;

RWStructuredBuffer<uint32_t> light_grid;

[[vk::push_constant]]
float3 grid_center;

groupshared uint32_t group_buckets[lgrid::WORD_COUNT];

[shader("compute")]
[numthreads(4, 4, 4)]
void main(int3 thread_id: SV_DispatchThreadID, int3 group_id: SV_GroupID, int group_i: SV_GroupIndex) {
    /* Retrieve current cascade and z coord */
    const uint32_t cascade_index = thread_id.z >> 4u; /* cascade_index = thread_id.z / 16u */
    const uint32_t z_coord = thread_id.z & 0b1111u;   /* z_coord = thread_id.z % 16u */

    /* Compute the cascade AABB (16x16x16 cells) */
    const uint32_t cascade_half_extent = (lgrid::FIRST_CASCADE_BOUNDS << cascade_index) >> 1;
    const Aabb cascade_aabb = Aabb(grid_center - cascade_half_extent, grid_center + cascade_half_extent);

    /* Compute the group AABB (4x4x4 cells) */
    const uint32_t group_extent = cascade_half_extent >> 1; /* cascade_half_extent / 2u */
    const uint3 local_group = uint3(group_id.x, group_id.y, group_id.z & 0b11u /* group_id.z % 4 */);
    const Aabb group_aabb = Aabb(cascade_aabb.min + group_extent * local_group, cascade_aabb.min + group_extent * (local_group + 1u));

    /* 1. 16x16x16 -> 4x4x4 culling. Cull against 4x4x4 groups (64 threads) */
    /* Cascades Flat Bit array loop */
    for (uint32_t i = 0u; i < lgrid::WORD_COUNT; i++) {
        /* Retrieve bitmasks for this cascade */
        uint32_t cascade_bitmask = cascades_bitmasks[cascade_index * lgrid::WORD_COUNT + i];
        group_buckets[i] = 0u; /* Initialize LDS */

        /* Loop over each bit in the bitmask (1 bit = 1 light) */
        while (cascade_bitmask != 0u) {
            /* Retrieve bit index */
            const uint32_t bit_index = firstbitlow(cascade_bitmask);

            /* Remove bit from mask */
            cascade_bitmask ^= (1u << bit_index);

            /* Get light */
            const uint32_t light_index = i * lgrid::WORD_COUNT + bit_index;
            const Light light = lights[light_index];

            /* Cull picked light against 4x4x4 group */
            /* Create light aabb */
            const Aabb light_aabb = Aabb(light.origin - light.culling_radius, light.origin + light.culling_radius);

            /* Update group bitmask */
            if (group_aabb.overlap(light_aabb)) {
                group_buckets[i] |= 1u << bit_index;
            }
        }
    }

    /* Wait for all groups to finish */
    GroupMemoryBarrierWithGroupSync();

    /* Compute cell aabb */
    const uint32_t cell_extent = group_extent >> 2u; /* group_extent / 4u */
    const uint3 local_cell = uint3(thread_id.x, thread_id.y, z_coord);
    const float3 cell_center = cascade_aabb.min + cell_extent * (float3(local_cell) + 0.5f);
    const float cell_radius = length(float3(cell_extent, cell_extent, cell_extent)) * 0.5f;

    /* 2. 4x4x4 -> 1x1x1 culling. Cull against individual cells in the light grid (1 thread = 1 cell) */
    for (uint32_t i = 0u; i < lgrid::WORD_COUNT; i++) {
        uint32_t group_bitmask = group_buckets[i];

        /* Loop over each bit in the group bitmask */
        while (group_bitmask != 0u) {
            /* Retrieve bit index */
            const uint32_t bit_index = firstbitlow(group_bitmask);

            /* Remove bit from mask */
            group_bitmask ^= (1u << bit_index);

            /* Get light index */
            const uint32_t light_index = i * lgrid::WORD_COUNT + bit_index;

            /* Insert light index into light_grid */
            if (fine_cull_light(lights[light_index], cell_center, cell_radius)) {
                /* Retrieve the cell base index */
                const uint32_t cell_base = lgrid::cell_base(cascade_index, thread_id.x, thread_id.y, z_coord);

                /* Read the cell count, early out if this cell is full */
                const uint32_t cell_count = light_grid[cell_base];
                if (cell_count >= lgrid::CELL_MAX_LIGHTS) return;

                /* Append light into cell */
                light_grid[cell_base] += 1u; /* Increment light count */
                light_grid[cell_base + cell_count + 1u] = light_index;
            }
        }
    }
}
