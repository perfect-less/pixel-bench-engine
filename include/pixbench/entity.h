#ifndef ENTITY_HEADER
#define ENTITY_HEADER

#include "pixbench/engine_config.h"
#include <bitset>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>


class EntityManager;
class IComponent;
class ISystem;

typedef size_t EntityIDNumber;

struct EntityID {
    EntityIDNumber id;
    size_t version = 0;

    bool operator==(EntityID const& other) const {
        return (this->id == other.id) && (this->version == other.version);
    }

    bool operator!=(EntityID const& other) const {
        return (this->id != other.id) || (this->version != other.version);
    }
};


struct EntityInfo {
    EntityID entityid;                              //!< Unique entity's identifier
    bool active = false;                            //!< active status, true means this entity is active
    size_t current_version = 0;                     //!< version, to prevent old disabled id to be used by user
    std::bitset<MAX_COMPONENTS> component_mask;     //!< mask of assigned components to this entity
    std::bitset<MAX_TAGS> tag_mask;                 //!< mask of assigned tags
};


class EntityTagAPI {
private:
    std::unordered_map<std::string, int> m_tag_to_index_map;
    std::string m_index_to_tag_array[MAX_TAGS];
    EntityManager* m_entity_manager = nullptr;
    EntityInfo* m_ent_mgr_entities = nullptr;       //!< entityManager's m_entities
    int tag_index_counter = 0;
public:

    void __setEntityManager(EntityManager* entityManager) {
        this->m_entity_manager = entityManager;
    }

    void __setEntityInfoArray(EntityInfo* entities) {
        m_ent_mgr_entities = entities;
    }

    int getTagIndex(std::string tag);

    int numOfEntityWithTag(std::string tag);

    void addTagToEntity(EntityID entity, std::string tag);

    void removeTagFromEntity(EntityID entity, std::string tag);

    bool isEntityHasTag(EntityID entity, std::string tag);

    std::vector<EntityID> getEntitiesWithTag(std::string tag);

    std::vector<std::string> getEntityTags(EntityID);
};


#endif
