#include "pixbench/game.h"
#include "pixbench/ecs.h"
#include "pixbench/hierarchy.h"
#include "pixbench/components.h"
#include "pixbench/entity.h"
#include "pixbench/physics/physics.h"
#include "pixbench/systems.h"
#include <cassert>
#include <memory>
#include <vector>

void HierarchyAPI::addChildToEntity(EntityID parent, EntityID child) {
    assert(game && "game shouldn't be null");
    auto hie_sys = std::static_pointer_cast<HierarchySystem>(game->hierarchySystem);
    assert(hie_sys && "game::hierarchySystem shouldn't be null");

    hie_sys->_addChildToEntity(parent, child);
}

void HierarchyAPI::addChildsToEntity(EntityID parent, std::vector<EntityID> childs) {
    assert(game && "game shouldn't be null");
}

void HierarchyAPI::removeChildFromEntity(EntityID parent, EntityID child) {
    assert(game && "game shouldn't be null");
    auto hie_sys = std::static_pointer_cast<HierarchySystem>(game->hierarchySystem);
    assert(hie_sys && "game::hierarchySystem shouldn't be null");

    hie_sys->_removeChildFromEntity(parent, child);
}

size_t HierarchyAPI::getEntityChildCount(EntityID parent) {
    Hierarchy* p_hie = game->entityManager->getEntityComponent<Hierarchy>(parent);
    
    if ( !p_hie )
        return 0;
    
    return p_hie->numChilds();
}

std::vector<EntityID> HierarchyAPI::getEntityChilds(EntityID parent, bool recursive) {
    assert(game && "game shouldn't be null");
    auto hie_sys = std::static_pointer_cast<HierarchySystem>(game->hierarchySystem);
    assert(hie_sys && "game::hierarchySystem shouldn't be null");

    std::vector<EntityID> childs;
    hie_sys->_getEntityChilds(parent, childs, recursive);
    
    return childs;
}


void HierarchyAPI::syncEntityTransform(EntityID parent) {
    assert(game && "game shouldn't be null");
    auto hie_sys = std::static_pointer_cast<HierarchySystem>(game->hierarchySystem);
    assert(hie_sys && "game::hierarchySystem shouldn't be null");

    hie_sys->_syncEntityTransform(parent);
}
