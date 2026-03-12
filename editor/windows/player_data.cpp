#include "player_data.hpp"
#include "engine/engine.hpp"
#include "engine/tools/player_data.hpp"
#include <ImReflect.hpp>

namespace tmt {

void PlayerDataWindow::display() {
    ImGui::Text("This window contains all player data that has been accesed this session.");

    if (ImGui::Button("Clear Player Data")) {
        engine.player_data.clear();
    }

    ImGui::SeparatorText("Data:");
    engine.player_data.inspect();
}

}  // namespace tmt