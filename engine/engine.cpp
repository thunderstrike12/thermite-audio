#include "engine.hpp"

#include "core/logger.hpp"

/* Singleton */
tmt::Engine tmt::engine;

namespace tmt {

Engine::Engine() {}

Engine::~Engine() {}

void Engine::init() {
    Log::init();
    tmt::Log::info("No scope here!");
    tmt::Log::warn(Log::LoggerScope::GAME, "Game scope here!");
    tmt::Log::warn(Log::LoggerScope::RENDERER, "Renderer scope here!");
    tmt::Log::warn(Log::LoggerScope::ENGINE, "THERMITE scope here!");
}

void Engine::run() {}

void Engine::end() {}

}  // namespace tmt
