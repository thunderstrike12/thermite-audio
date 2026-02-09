#pragma once
#include "editor/core/window.hpp"

namespace tmt {

/* Forward Declare */
class UndoRedoManager;

class IUndoRedo {
   public:
    virtual ~IUndoRedo() = default;
    virtual void undo() = 0;
    virtual void redo() = 0;

    virtual void inspect() = 0;

   protected:
    template <typename Derived>
    void send_to_manager(Derived&& action, const std::string& message) {
        auto shared_action = std::make_shared<std::decay_t<Derived>>(std::forward<Derived>(action));
        get_manager().commit_action(shared_action, message);
    };

   private:
    UndoRedoManager& get_manager();
};

class UndoRedoCollection : public IUndoRedo {
   public:
    UndoRedoCollection() = default;
    ~UndoRedoCollection() override = default;

    template <typename Derived>
    void add_action(Derived&& action) {
        auto shared_action = std::make_shared<std::decay_t<Derived>>(std::forward<Derived>(action));
        actions.push_back(shared_action);
    }
    void commit(const std::string& message);

    // Inherited via IUndoRedo
    void undo() override;
    void redo() override;
    void inspect() override;

   private:
    std::vector<std::shared_ptr<IUndoRedo>> actions;
};

class UndoRedoManager : public IWindow {
   public:
    UndoRedoManager() = default;
    ~UndoRedoManager() override = default;

    void commit_action(const std::shared_ptr<IUndoRedo>& action, const std::string& message);

   protected:
    // Inherited via IWindow
    std::string get_title() const override { return "Undo Redo Manager"; }

    void display() override;

    void on_editor_start() override;
    void on_editor_update(const tmt::FrameData& time) override;
    void on_editor_end() override;

   private:
    struct CommitAction {
        const std::string message;
        std::shared_ptr<IUndoRedo> action;
    };

    std::vector<CommitAction> undo_stack;
    std::vector<CommitAction> redo_stack;

    void undo();
    void redo();
};

}  // namespace tmt