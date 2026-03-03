#pragma once

#include <set>

#include <engine/core/ecs.hpp>

#include "editor/core/systems/undo_redo/undo_redo_manager.hpp"

namespace tmt {

/**
 * Diff for entity creation and deletion in the hierarchy.
 * Serializes entity data to JSON so entities can be recreated on undo/redo.
 *
 * When `is_creation` is true, undo destroys and redo recreates.
 * When `is_creation` is false (deletion), undo recreates and redo destroys.
 */
class EntityDiff : public IUndoRedo {
   public:
    /**
     * Construct a diff for a single entity.
     * @param entity The entity being created or deleted.
     * @param is_creation True if the action was creating the entity, false if deleting.
     */
    EntityDiff(Entity entity, bool is_creation);

    /**
     * Construct a diff for multiple entities (e.g. paste/duplicate).
     * @param entities The set of entities being created or deleted.
     * @param is_creation True if the action was creating the entities, false if deleting.
     */
    EntityDiff(const std::set<Entity>& entities, bool is_creation);

    // Inherited via IUndoRedo
    void undo() override;
    void redo() override;
    void inspect() override {}

   private:
    bool is_creation { false };

    json entity_json;
    std::set<Entity> entity_ids;

    void destroy_entities();
    void recreate_entities();
};

}  // namespace tmt
