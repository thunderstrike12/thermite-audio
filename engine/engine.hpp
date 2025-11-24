#pragma once

#include <string>

namespace tmt {

#if THERMITE_EDITOR
#pragma message(" THERMITE_EDITOR=1 ")
#else
#pragma message(" THERMITE_EDITOR=0 ")
#endif

#if THERMITE_DEBUG
#pragma message("THERMITE_DEBUG=1")
#else
#pragma message("THERMITE_DEBUG=0")
#endif

// Example engine class
class Engine {
   public:
    explicit Engine(std::string name);

    void run();
    const std::string& get_name() const;

   private:
    std::string name {};
};

}  // namespace tmt
