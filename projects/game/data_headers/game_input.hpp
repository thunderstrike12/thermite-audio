#pragma once

namespace game {
namespace action {

// Movement
constexpr auto MOVE_FORWARD = "move_forward";
constexpr auto MOVE_BACKWARD = "move_backward";
constexpr auto MOVE_LEFT = "move_left";
constexpr auto MOVE_RIGHT = "move_right";
constexpr auto MOVE_UP = "move_up";
constexpr auto MOVE_DOWN = "move_down";
constexpr auto BREAK = "break";
constexpr auto BOOST = "boost";

// Tools and guns
constexpr auto SHOOT = "shoot";
constexpr auto SWITCH_RIFLE = "rifle";
constexpr auto SWITCH_MINING = "mining";
constexpr auto SWITCH_GRAVITY = "gravity";
constexpr auto SECONDARY_TOOL_USE = "secondary_tool_use";

// Menus
constexpr auto OPEN_PAUSE_MENU = "open_pause_menu";
constexpr auto OPEN_INVENTORY = "open_inventory";
constexpr auto OPEN_UPGRADE_MENU = "open_upgrade_menu";

// Barge
constexpr auto ATTACH_KEY = "attach_key";
constexpr auto TRIGGER_BARGE_MOVEMENT = "trigger_barge_movement";
constexpr auto TRIGGER_RUN_END = "trigger_run_end";

}  // namespace action
}  // namespace game
