#ifndef GAME_MANAGER_HEADER
#define GAME_MANAGER_HEADER

#include "pixbench/components.h"
#include "pixbench/ecs.h"

class GameManager : public ScriptComponent {
private:
    Game* game = nullptr;
    EntityManager* entity_mgr = nullptr;
public:
    Result<VoidResult, GameError> Init(Game *game, EntityManager *entityManager, EntityID self) override;

    Result<VoidResult, GameError> OnEvent(SDL_Event *event, EntityManager *entityManager, EntityID self) override;
    
    Result<VoidResult, GameError> Update(double deltaTime_s, EntityManager *entityManager, EntityID self) override;

    void quitGame();
};


#endif
