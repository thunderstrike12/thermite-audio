#pragma once

namespace tmt {
class Window;

class Engine {
   public:
    Engine();
    ~Engine();

    /* No copies allowed */
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    void init();
    void run();
    void end();

    /*
    How to declare a system:
    MySystem& my_system;

    Then in constructor:

    Engine() : my_system(*new MySystem()) {};

    **IMPORTANT**
    Also in the destructor

    ~Engine() { delete &my_system; };
    */

    Window& window;
};

/* Singleton */
extern Engine engine;

}  // namespace tmt

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
