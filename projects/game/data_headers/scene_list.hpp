#pragma once
#include "engine/core/scene.hpp"
#include "engine/core/scenes.hpp"

class MainMenuScene : public tmt::Scene<MainMenuScene> {
   public:
    static constexpr std::string_view scene_name() { return "MainMenuScene"; }
};

class MainGameScene : public tmt::Scene<MainGameScene> {
   public:
    static constexpr std::string_view scene_name() { return "MainGameScene"; }
};

class HubScene : public tmt::Scene<MainGameScene> {
   public:
    static constexpr std::string_view scene_name() { return "HubScene"; }
};

class Zoo : public tmt::Scene<Zoo> {
   public:
    static constexpr std::string_view scene_name() { return "Zoo"; }
};
class Gym : public tmt::Scene<Gym> {
   public:
    static constexpr std::string_view scene_name() { return "Gym"; }
};

class DanielTestScene : public tmt::Scene<DanielTestScene> {
   public:
    static constexpr std::string_view scene_name() { return "DanielTestScene"; }
};

class MikaTestScene : public tmt::Scene<MikaTestScene> {
   public:
    static constexpr std::string_view scene_name() { return "MikaTestScene"; }
};

class LoekTestScene : public tmt::Scene<LoekTestScene> {
   public:
    static constexpr std::string_view scene_name() { return "LoekTestScene"; }
};