#pragma once

#include <cstdint>

namespace tmt {

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

}  // namespace tmt
