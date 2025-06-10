#include "pixbench/ecs.h"
#include "pixbench/game.h"
#include "pixbench/renderer.h"
#include "pixbench/resource.h"
#include "pixbench/utils.h"
#include "pixbench/vector2.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_joystick.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_surface.h>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
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
    gConfig.render_clear_color   = Color(255, 255, 100, 255);

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

    // create entity
    EntityID ent = ent_mgr->createEntity();

    // adding component to entity
    Transform* transform = ent_mgr->addComponentToEntity<Transform>(ent);

    // creating your custom script as a component
    class QuitHandlerScript : public ScriptComponent {
    public:
        Game* m_game = nullptr; // object reference to pointer
        
        // Init function called when entity begin its life
        void Init(Game *game, EntityManager *entityManager, EntityID self) override {
            m_game = game;
        }

        // You can add your own function
        void quitTheGame() {
            std::cout << "QuitHandlerScript::quitTheGame called, will quit the game" << std::endl;
            if (m_game)
                m_game->Quit();
        }

        void OnDestroy(EntityManager *entityManager, EntityID self) override {
            std::cout << "QuitHandlerScript::OnDestroy";
            std::cout << " entity->" << self.id << " called to be destroyed";
            std::cout << std::endl;
        }

        // OnEvent called everytime SDL report an event
        void OnEvent(SDL_Event *event, EntityManager *entityManager, EntityID self) override {
            // Handle quit
            if (event->type == SDL_EVENT_QUIT) {
                quitTheGame();
            }

            if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_Q) {
                quitTheGame();
            }
        }
    };
    
    // then you can add that component
    ent_mgr->addComponentToEntity<QuitHandlerScript>(ent);

    // destroying entity
    // ent_mgr->destroyEntity(ent);

    std::cout << "InitializeGame done" << std::endl;
}
