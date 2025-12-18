#pragma once

#include <cstdint>

namespace tmt {

// these are SDL values, which might be wrong. This is done like this so SDL include can be avoided.
enum class Key : int32_t {
    UNKNOWN = 0,

    // Letters (USB HID: 0x04-0x1D)
    A = 4,
    B = 5,
    C = 6,
    D = 7,
    E = 8,
    F = 9,
    G = 10,
    H = 11,
    I = 12,
    J = 13,
    K = 14,
    L = 15,
    M = 16,
    N = 17,
    O = 18,
    P = 19,
    Q = 20,
    R = 21,
    S = 22,
    T = 23,
    U = 24,
    V = 25,
    W = 26,
    X = 27,
    Y = 28,
    Z = 29,

    // Numbers (USB HID: 0x1E-0x27)
    NUM_1 = 30,
    NUM_2 = 31,
    NUM_3 = 32,
    NUM_4 = 33,
    NUM_5 = 34,
    NUM_6 = 35,
    NUM_7 = 36,
    NUM_8 = 37,
    NUM_9 = 38,
    NUM_0 = 39,

    RETURN = 40,
    ESCAPE = 41,
    BACKSPACE = 42,
    TAB = 43,
    SPACE = 44,

    // Function keys (USB HID: 0x3A-0x45)
    F1 = 58,
    F2 = 59,
    F3 = 60,
    F4 = 61,
    F5 = 62,
    F6 = 63,
    F7 = 64,
    F8 = 65,
    F9 = 66,
    F10 = 67,
    F11 = 68,
    F12 = 69,

    // Arrow keys
    RIGHT = 79,
    LEFT = 80,
    DOWN = 81,
    UP = 82,

    // Modifiers
    LEFT_CTRL = 224,
    LEFT_SHIFT = 225,
    LEFT_ALT = 226,
    RIGHT_CTRL = 228,
    RIGHT_SHIFT = 229,
    RIGHT_ALT = 230,
};
enum class GamepadButton : int32_t {
    SOUTH = 0,  // A / Cross
    EAST = 1,   // B / Circle
    WEST = 2,   // X / Square
    NORTH = 3,  // Y / Triangle
    BACK = 4,
    GUIDE = 5,
    START = 6,
    LEFT_STICK = 7,
    RIGHT_STICK = 8,
    LEFT_SHOULDER = 9,
    RIGHT_SHOULDER = 10,
    DPAD_UP = 11,
    DPAD_DOWN = 12,
    DPAD_LEFT = 13,
    DPAD_RIGHT = 14,
    MISC1 = 15,
    RIGHT_PADDLE1 = 16,
    LEFT_PADDLE1 = 17,
    RIGHT_PADDLE2 = 18,
    LEFT_PADDLE2 = 19,
    TOUCHPAD = 20,
    COUNT = 21,
};

enum class GamepadAxis : int32_t {
    INVALID = -1,
    LEFT_X = 0,
    LEFT_Y = 1,
    RIGHT_X = 2,
    RIGHT_Y = 3,
    LEFT_TRIGGER = 4,
    RIGHT_TRIGGER = 5,
    COUNT = 6,
};
struct GamepadAxisDirection {
    GamepadAxis axis;
    bool negative = false;
};
enum class MouseButton : int32_t {
    LEFT = 1,
    MIDDLE = 2,
    RIGHT = 3,
    X1 = 4,
    X2 = 5,
};

}  // namespace tmt
