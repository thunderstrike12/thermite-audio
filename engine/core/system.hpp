#pragma once
#include <string>

#include "engine/core/frame_data.hpp"

#include "engine/events/game.hpp"

namespace tmt {

/* Interface */
class ISystem : public IGameEvents {
   public:
    virtual ~ISystem() = default;

    /* [Required] */
    constexpr virtual std::string get_name() = 0;
};

}  // namespace tmt