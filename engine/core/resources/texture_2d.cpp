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

    data = stbi_load(file_location.get_absolute_path().string().c_str(), &tex_width, &tex_height, &channels, 4);
    if (!data) {
        return false;
    }

    width = (uint32_t)tex_width;
    height = (uint32_t)tex_height;

    auto& bank = engine.renderer.vram_bank();

    /* Initialise the texture */
    std::string texture_name = name + " Texture";
    if (const Result r = bank.create_texture(texture_name.c_str(), TextureUsage::Sampled | TextureUsage::TransferDst, TextureFormat::RGBA8Unorm, {(u32)tex_width, (u32)tex_height, 0});
        r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialise texture.\nreason: {}", r.unwrap_err().c_str());
        return false;
    } else
        texture = r.unwrap();

    if (const Result r = bank.upload_texture(texture, data, tex_width * tex_height * 4); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to upload texture.\nreason: {}", r.unwrap_err().c_str());
        return false;
    }
    free(data);

    std::string image_name = name + " Image";
    if (const Result r = bank.create_image(image_name.c_str(), texture); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialize image.\nreason: {}", r.unwrap_err().c_str());
        return false;
    } else
        image = r.unwrap();

    return true;
}

void Texture2D::unload() {
    auto& bank = engine.renderer.vram_bank();

    bank.destroy(texture);
    bank.destroy(image);
}

}  // namespace tmt
