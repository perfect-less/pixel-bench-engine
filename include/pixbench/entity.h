#ifndef ENTITY_HEADER
#define ENTITY_HEADER

#include "pixbench/engine_config.h"
#include <bitset>
#include <cstddef>


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
};


#endif
