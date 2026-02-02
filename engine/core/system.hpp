#pragma once
#include <string>

#include "engine/core/frame_data.hpp"
#include "engine/events/game.hpp"
#include "engine/tools/serializer.hpp"

namespace tmt {

/* Interface */
class ISystem : public IGameEvents {
   public:
    virtual ~ISystem() = default;

    /* [Required] */
    constexpr virtual std::string get_name() = 0;
    /* [Optional] */
    virtual json serialize() const { return json::object(); }
    virtual void deserialize(const json& value) { (void)value; }
};

}  // namespace tmt