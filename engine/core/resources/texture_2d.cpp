#include "texture_2d.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <graphite/vram_bank.hh>

#include "engine.hpp"

#include "core/logger.hpp"
#include "core/renderer/renderer.hpp"

namespace tmt {

bool Texture2D::load() {
    int tex_width = -1;
    int tex_height = -1;
    int channels = -1;
    unsigned char* data = nullptr;

    /* Try to load the texture */
    data = stbi_load(file_location.get_absolute_path().string().c_str(), &tex_width, &tex_height, &channels, 4);
    if (!data) {
        return false;
    }

    /* Set the texture properties */
    width = (uint32_t)tex_width;
    height = (uint32_t)tex_height;
    name = file_location.get_relative_path().string();

    /* Get the VRAM bank */
    auto& bank = engine.renderer.vram_bank();

    /* Initialise the texture */
    std::string texture_name = name + " Texture";
    texture = bank.create_texture(texture_name.c_str(), TextureUsage::Sampled | TextureUsage::TransferDst, TextureFormat::RGBA8Unorm, { (u32)tex_width, (u32)tex_height, 0 })
                  .expect("failed to initialise texture.");
    bank.upload_texture(texture, data, tex_width * tex_height * 4).expect("failed to upload texture.");
    free(data);

    /* Initialise the image */
    std::string image_name = name + " Image";
    image = bank.create_image(image_name.c_str(), texture).expect("failed to initialize image.");

    return true;
}

void Texture2D::unload() {
    engine.renderer.destroy(texture);
    engine.renderer.destroy(image);
}

}  // namespace tmt
