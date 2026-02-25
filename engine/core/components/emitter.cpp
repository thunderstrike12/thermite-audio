#include "emitter.hpp"

#include "engine.hpp"
#include "core/resources.hpp"

namespace tmt {

ParticleEffect::ParticleEffect() {
    // Load a default texture
    texture = tmt::engine.resources.load_resource<tmt::Texture2D>({ tmt::IO::Location::ENGINE, "missing.png" });
}

}  // namespace tmt