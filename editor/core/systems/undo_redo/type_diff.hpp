#pragma once

#include "editor/core/systems/undo_redo/undo_redo_manager.hpp"

namespace tmt {

template <typename Type>
class TypeDiff : public IUndoRedo {
   public:
    TypeDiff(Type* variable) : variable { variable } {}

    void before() { before_change = *variable; }
    void after() { after_change = *variable; }

    void undo() override { *variable = before_change; }
    void redo() override { *variable = after_change; }

    void inspect() override {}

   private:
    Type* variable;

    Type before_change;
    Type after_change;
};

}  // namespace tmt