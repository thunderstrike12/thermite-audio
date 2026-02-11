#include "emitter.hpp"

#include "engine.hpp"
#include "core/resources.hpp"

namespace tmt {

ParticleEmitter::ParticleEmitter() {
    // Load a default texture
    texture = tmt::engine.resources.load_resource<tmt::Texture2D>({ tmt::IO::Location::ENGINE, "missing.png" });
}

}  // namespace tmt