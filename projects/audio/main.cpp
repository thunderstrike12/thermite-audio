#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/resource.hpp"
#include "engine/core/io.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/scene.hpp"
#include "engine/core/scenes.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/components/audio_emitter.hpp"

namespace {

tmt::Entity entity;

}  // namespace

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}
};

class TestScene : public tmt::Scene<TestScene> {
   public:
    static std::string_view scene_name() { return "TestScene"; }

    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void on_end() override;
};

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    tmt::ApplicationSpecs specs {
        .name = "Audio Module Test",
        .command_args = args,
        .log_file = "audio.txt",
    };

    tmt::engine.scenes.register_scene<TestScene>();

    return std::make_unique<Game>(specs);
}

void TestScene::on_start() {
    // Just get the first entity with the AudioEmitter component, there is only 1 in the saved scene.
    entity = tmt::engine.ecs.get_registry().view<tmt::AudioEmitter>().front();
}

void TestScene::on_update(const tmt::FrameData& time) {
    const float n = time.elapsed_time;
    const glm::vec3 position = glm::vec3 {glm::cos(n), 0.0f, glm::sin(n)} * 3.0f;

    tmt::Transform& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    transform.set_world_position(position);
}

void TestScene::on_end() {}
