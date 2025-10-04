#include "demo/platformer/include/game_manager.h"
#include "pixbench/ecs.h"
#include "pixbench/entity.h"
#include "pixbench/game.h"
#include "pixbench/utils/utils.h"
#include "include/chars.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_joystick.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_surface.h>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <string>


Game* Game::CreateGame() {
    /* Create a Game object,
     * the lifetime of this object is the lifetime of the
     * application */
    Game* game = new Game();

    /* Configuring the game */
    GameConfig gConfig = GameConfig();
    gConfig.game_title      = "Pixel Node - Game";
    gConfig.game_version    = "0.0.1";
    gConfig.game_identifier = "com.example.pixelnode-game";

    gConfig.render_vsync_enabled = true;
    gConfig.render_clear_color   = Color(200, 200, 200, 255);

    game->ApplyGameConfig(gConfig);

    /* Initialize Game (create renderer, window, etc.) */
    Result<VoidResult, GameError> res = game->Initialize();
    if ( res.isError() ) {
        /* error checking to make sure game initialization actually runs
         * successfuly
         */
        std::cout << "Can't Initialize Game: "
            << res.getErrResult()->err_message
            << std::endl;
        return nullptr;  // returning null will tell the engine to stop
                         // the application.
    }

    return game;
}

void Game::InitializeGame(Game* game) {
    std::cout << "InitializeGame called" << std::endl;
    auto ent_mgr = game->entityManager;

    // create game manager entity
    EntityID ent = ent_mgr->createEntity();
    GameManager* game_mgr = ent_mgr->addComponentToEntity<GameManager>(ent);

    spawnPlayableCharacter(ent_mgr);

    std::cout << "InitializeGame done" << std::endl;
}
