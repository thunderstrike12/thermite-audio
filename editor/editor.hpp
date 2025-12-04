#pragma once

namespace tmt {
class Editor {
   public:
    Editor();
    ~Editor();
    void start();
};

/* Singleton */
extern Editor editor;

}  // namespace tmt