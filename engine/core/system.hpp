#pragma once
#include <string>

#include "frame_data.hpp"

namespace tmt {

/* Interface */
class ISystem {
   public:
    virtual ~ISystem() = default;

    /* [Required] */
    constexpr virtual std::string get_name() = 0;

    /* Events */
    /* [Required] */
    virtual void on_start() = 0;
    virtual void on_update(const FrameData& time) = 0;
    virtual void on_end() = 0;
    /* [Optional] */
    virtual void on_fixed_update(const FrameData& time) {};
};

}  // namespace tmt