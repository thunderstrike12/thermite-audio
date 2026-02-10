module bindless;

/* Bindless descriptors */
[[vk::binding(0, 1)]] __DynamicResource<__DynamicResourceKind.General> bindless_sampled_textures[];
[[vk::binding(1, 1)]] __DynamicResource<__DynamicResourceKind.General> bindless_storage_buffers[];

namespace bindless {

/* Mark index as non-uniform using SPIR-V assembly. */
[ForceInline] T non_uniform_index<T>(T index) {
    __target_switch {
        case spirv:
            const T index_copy = __copyObject(index);
            spirv_asm {
                OpCapability ShaderNonUniform;
                OpDecorate $index NonUniform;
                OpDecorate $index_copy NonUniform;
            };
            return index_copy;
        default: return index;
    }
}

/**
 * Handles are 28 index bits, and 4 type bits.
 * That's why we mask away the 4 type bits.
 * Index 0 signals an invalid handle, that's why we subtract 1.
 */

/* Get a bindless texture with a given handle. */
public [ForceInline] T texture_sampled<T>(uint handle) where T : IOpaqueDescriptor {
    return bindless_sampled_textures[non_uniform_index((handle & 0x0FFFFFFF) - 1u)].asOpaqueDescriptor<T>();
}

/* Get a bindless storage buffer with a given handle. */
public [ForceInline] T storage_buffer<T>(uint handle) where T : IOpaqueDescriptor {
    return bindless_storage_buffers[non_uniform_index((handle & 0x0FFFFFFF) - 1u)].asOpaqueDescriptor<T>();
}

} // bindless
