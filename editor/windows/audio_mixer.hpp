#pragma once

#include <variant>

#include <imgui.h>
#include <engine/core/audio.hpp>

#include "editor/core/window.hpp"

namespace tmt {

class AudioEvent;
class AudioBank;

class AudioMixer : public IWindow, public OnGameEnd {
   public:
    AudioMixer() = default;
    ~AudioMixer() override = default;

    void on_inspect() override;

    [[nodiscard]] constexpr std::string get_title() const override { return ICON_MS_AUDIOTRACK " Audio Mixer"; }

    void on_editor_start() override {}
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override {}

   private:
    constexpr int get_window_flags() const override { return ImGuiWindowFlags_MenuBar; }
    void on_game_end() override;
    void display_menu_bar();

    // Template function is defined in the .cpp file, INTENTIONALLY making it only usable in that cpp file.
    template <typename Type>
    int get_selection_flags(const Type& selection_compare);
    template <>
    int get_selection_flags<std::weak_ptr<AudioBank>>(const std::weak_ptr<AudioBank>& selection_compare);

    void invalidate_selection();
    // Selection can be any of these 3 audio types, so an std::variant gives us a convenient way of checking what type is selected and letting us access that type.
    std::variant<std::weak_ptr<AudioBank>, AudioEvent, VolumeControl> selection = AudioEvent {};
    std::vector<ResourceRef<AudioBank>> cached_banks;
};

}  // namespace tmt