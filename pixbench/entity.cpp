#include "pixbench/ecs.h"
#include <algorithm>
#include <ostream>
#include <vector>


EntityManager::EntityManager() {
    this->m_component_manager = new ComponentManager();
    
    for (int i = 0; i < MAX_ENTITIES; i++) {
        this->m_entities[i].entityid.id = i;
        this->m_entities[i].entityid.version = 0;
        this->m_entities[i].active = false;
        this->m_entities[i].current_version = 0;
        this->m_entities[i].component_mask.reset();
    }
};


EntityManager::~EntityManager() {
    delete (this->m_component_manager);
};


void EntityManager::setComponentRegisterCallback(
        std::function<void(ComponentTag, ComponentType, size_t)> callback_func
        ) {
    m_component_manager->component_registered_callback = callback_func;
}


void EntityManager::setOnEntityDestroyedCallback(
        std::function<void(EntityID entity_id)> callback_func
        ) {
    m_on_entity_destroyed_callback = callback_func;
}


EntityID EntityManager::createEntity() {
    EntityID new_id;
    EntityIDNumber new_id_index;

    if (this->m_unused_entity_queue.size() > 0) {
        new_id_index = this->m_unused_entity_queue.front();
        this->m_unused_entity_queue.pop();
    } else {
        new_id_index = this->m_last_available_entity_number;
        ++(this->m_last_available_entity_number);
    }

    new_id.id = new_id_index;
    new_id.version = this->m_entities[new_id.id].current_version;
    this->m_entities[new_id.id].active = true;
    this->m_entities[new_id.id].entityid.version = this->m_entities[new_id.id].current_version;
    this->m_uninitialized_entities.push_back(new_id);
    return new_id;
}


std::vector<EntityID> EntityManager::getEntities() {
    // TO DO
    // return this->entities;
}


std::vector<EntityID> EntityManager::getUninitializedEntities() {
    return this->m_uninitialized_entities;
}


void EntityManager::setEntityAsInitialized(EntityID entity) {
    this->m_uninitialized_entities.erase(
            std::remove_if(
                this->m_uninitialized_entities.begin(),
                this->m_uninitialized_entities.end(),
                [entity](const EntityID& ent) {
                    return entity.id == ent.id;
                }),
            this->m_uninitialized_entities.end()
            );
}


void EntityManager::resetEntitiesUninitializedStatus() {
    this->m_uninitialized_entities.clear();
}


// std::vector<EntityID> EntityManager::GetEntitiesByTag(std::string tag) {
//     auto entities_it = this->tag_to_entities_map.find(tag);
//     if (entities_it != this->tag_to_entities_map.end()) {
//         return entities_it->second;
//     }
//
//     return std::vector<EntityID>();  // empty vector
// };
