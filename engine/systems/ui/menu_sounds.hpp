#pragma once
#include "engine/core/audio.hpp"

namespace tmt {

struct MenuSounds {
    tmt::AudioEvent button_hover;        // done
    tmt::AudioEvent button_click;        // done
    tmt::AudioEvent slider_move;         // done
    tmt::AudioEvent button_accept;       // done
    tmt::AudioEvent insufficient_funds;  // done
    tmt::AudioEvent money_gained;        // done
    tmt::AudioEvent loading;             // done, no looping
    tmt::AudioEvent open_menu;           // only esc menu
};

}  // namespace tmt

TMT_OBJECT(tmt::MenuSounds, (button_hover, button_accept, button_click, insufficient_funds, loading, money_gained, open_menu, slider_move));
