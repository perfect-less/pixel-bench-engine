#ifndef HIERARCHY_HEADER
#define HIERARCHY_HEADER

#include "pixbench/entity.h"
#include <unordered_map>
#include <vector>

class Game;

class HierarchyAPI {
private:
public:
    Game* game = nullptr;
    
    /**
     * Add `child` to `parent`
     */
    void addChildToEntity(EntityID parent, EntityID child);

    /**
     * Add multiple `child` to `parent` entity
     */
    void addChildsToEntity(EntityID parent, std::vector<EntityID> childs);


    /**
     * Remove entity from `parent`'s list of childs.
     */
    void removeChildFromEntity(EntityID parent, EntityID child);

    /**
     * Get a vector list of entities childs.
     */
    std::vector<EntityID> getEntityChilds(EntityID parent, bool recursive=false);

    /**
     * Get the number of child an entity has.
     */
    size_t getEntityChildCount(EntityID parent);


    /**
     * Sync entity's childs transforms.
     */
    void syncEntityTransform(EntityID parent);

};

#endif
