#include "engine/entry_point.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/resource.hpp"
#include "engine/core/io.hpp"
#include "engine/core/resources.hpp"

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
        .name = "Resource Loading Test",
        .command_args = args,
        .log_file = "resource_loading.txt"
    };
    // clang-format on

    return std::make_unique<Game>(specs);
}

class TextFile : public tmt::FileResource {
   public:
    TextFile(tmt::IO::FileLocation file_location) : FileResource(std::move(file_location)) {}

    // Inherited via FileResource
    bool load() override {
        original_content = tmt::IO::read_text_file(file_location);
        return true;
    }
    void unload() override {}

    std::string original_content;
};

class RuntimeTextFile : public tmt::RuntimeResource<TextFile> {
   public:
    RuntimeTextFile(const std::shared_ptr<TextFile>& file_resource) : RuntimeResource<TextFile>(file_resource) {}

    // Inherited via RunTimeRsource
    bool load() override {
        /* Modify it in someway */
        modified_content = file_resource->original_content + " - Modified at runtime";
        return true;
    }
    void unload() override { modified_content.clear(); }

    std::string modified_content;
};

void Game::on_start() {
    {
        /* load file resource */
        auto source = tmt::engine.resources.load_resource<TextFile>({tmt::IO::Location::PROJECT, "source.txt"});
        tmt::Log::info("{}", source->original_content);

        /* create runtime resource from file resource */
        auto modified = tmt::engine.resources.copy_resource<RuntimeTextFile>(source);
        tmt::Log::info("{}", modified->modified_content);

        /* create another runtime resource from the same file resource to test independence */
        auto also_modified = tmt::engine.resources.copy_resource<RuntimeTextFile>({tmt::IO::Location::PROJECT, "source.txt"});
        tmt::Log::info("{}", also_modified->modified_content);

        also_modified->modified_content += " - Further modified";
        if (also_modified->modified_content != modified->modified_content) {
            tmt::Log::info("Runtime resources are independent!");
        } else {
            throw std::runtime_error("Runtime resources are not independent!");
        }

        tmt::engine.resources.unload_unused();
        tmt::Log::info("Resource count before going out of scope: {}", tmt::engine.resources.resource_count());
    }
    /* Runtime resources go out of scope */

    tmt::engine.resources.unload_unused();
    tmt::Log::info("Resource count after going out of scope: {}", tmt::engine.resources.resource_count());
}

void Game::on_update(const tmt::FrameData& time) {}

void Game::on_end() {}
