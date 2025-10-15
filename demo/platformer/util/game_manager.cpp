#include "demo/platformer/include/game_manager.h"
#include "pixbench/components.h"
#include "pixbench/utils/results.h"


Result<VoidResult, GameError> GameManager::Init(Game *game, EntityManager *entityManager, EntityID self) {
    this->game = game;
    this->entity_mgr = game->entityManager;

    return ResultOK;
}

void GameManager::quitGame() {
    if (game)
        game->Quit();
}

Result<VoidResult, GameError> GameManager::OnEvent(SDL_Event *event, EntityManager *entityManager, EntityID self) {
    // Handle nullptr game
    if ( !game )
        return ResultOK;

    // Handle Quit
    const bool quit_commanded = (
            (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_Q)
            ||
            (event->type == SDL_EVENT_QUIT)
            );
    if (quit_commanded) {
        this->quitGame();
    }

    return ResultOK;
}


Result<VoidResult, GameError> GameManager::Update(double deltaTime_s, EntityManager *entityManager, EntityID self) {
    return ResultOK;
}
