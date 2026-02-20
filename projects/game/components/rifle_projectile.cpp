#include "rifle_projectile.hpp"
namespace game {

void RifleProjectile::start() {}
void RifleProjectile::update(const tmt::FrameData& time) {
    // TODO check for collision via raycast
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    auto delta = direction * time.delta_time * movement_speed;
    transform.translate(delta);
}
void RifleProjectile::end() {}
void RifleProjectile::collide() {
    // trigger collision event depending on what was hit
}

}  // namespace game
