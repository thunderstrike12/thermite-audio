#pragma once
#include "menu_sounds.hpp"

namespace tmt {

class UiAudioRegistry {
   public:
    MenuSounds sounds;

    void load();
    void save() const;
};

}  // namespace tmt

TMT_OBJECT(tmt::UiAudioRegistry, (sounds));
