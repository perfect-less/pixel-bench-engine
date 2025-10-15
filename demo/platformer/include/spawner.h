#ifndef SPAWNER_HEADER
#define SPAWNER_HEADER

#include "pixbench/ecs.h"
#include "pixbench/entity.h"
#include "pixbench/game.h"
#include <vector>


class Spawner {
private:
    std::vector<EntityID> m_entities;
public:
    Game* game;
    EntityManager* entityManager;

    Spawner(Game* game, EntityManager* entityManager) {
        this->game = game;
        this->entityManager = entityManager;
    }
    
    /**
     * spawn function to spawn entities
     */
    virtual std::vector<EntityID> spawn();

    /**
     * Initialize attributes of each entities
     */
    virtual void init();

    
    /**
     * returns spawners list of entities
     */
    std::vector<EntityID>& getEntities() {
        return m_entities;
    }

};

#endif
