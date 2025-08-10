#ifndef ENTITY_HEADER
#define ENTITY_HEADER

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


#endif
