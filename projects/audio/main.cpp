#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/resource.hpp"
#include "engine/core/io.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/resources/audio_bank.hpp"

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void on_end() override;
};

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    // clang-format off
    tmt::ApplicationSpecs specs {
        .name = "Audio Module Test",
        .command_args = args,
        .log_file = "audio.txt"
    };
    // clang-format on

    return std::make_unique<Game>(specs);
}

void Game::on_start() {
    tmt::engine.resources.load_resource<tmt::AudioBank>({tmt::IO::Location::PROJECT, "Master.bank"}, true);
    tmt::engine.resources.load_resource<tmt::AudioBank>({tmt::IO::Location::PROJECT, "Music.bank"});
    tmt::engine.resources.load_resource<tmt::AudioBank>({tmt::IO::Location::PROJECT, "SFX.bank"});
    tmt::engine.resources.load_resource<tmt::AudioBank>({tmt::IO::Location::PROJECT, "Vehicles.bank"});
    tmt::engine.resources.load_resource<tmt::AudioBank>({tmt::IO::Location::PROJECT, "VO.bank"});
}

void Game::on_update(const tmt::FrameData& time) {}

void Game::on_end() {}
