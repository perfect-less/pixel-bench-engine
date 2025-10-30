#ifndef PIXBENCH_INIT
#define PIXBENCH_INIT

#include "pixbench/game.h"

/**
 * **User defined function** that sould be overriden. This function
 * shall be used to create a Game object and calling the Game::Initialize()
 * function.
 * Example of the usage of this function
 * ~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * Game* CreateGame() {
 *     Game* game = new Game();
 * 
 *     GameConfig gConfig = GameConfig();
 *     gConfig.game_title      = "Pixel Node - Game";
 *     gConfig.render_vsync_enabled = true;
 *     gConfig.render_clear_color   = Color(255, 255, 100, 255);
 *     game->ApplyGameConfig(gConfig);
 * 
 *     game->Initialize();
 *     auto res = game->Initialize();
 *     if ( res.isError() ) {
 *         std::cout << "Can't Initialize Game: "
 *             << res.getErrResult()->err_message
 *             << std::endl;
 *         return nullptr;
 *     }
 * 
 *     return game;
 * }
 * ~~~~~~~~~~~~~~~~~~~~~~~~
 */
Game* CreateGame();

/**
 * **User defined function** that should be overriden by the user. You can
 * loads assets and then creates your entities here.
 * example usage: 
 * ~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * void InitializeGame(Game* game) {
 *     auto ent_mgr = game->entityManager;
 *     EntityID ent = ent_mgr->createEntity();
 * 
 *     Transform* transform = ent_mgr->addComponentToEntity<Transform>(ent);
 * 
 *     class QuitHandlerScript : public ScriptComponent {
 *     public:
 *         Game* m_game = nullptr; // object reference to pointer
 *         
 *         void Init(Game *game, EntityManager *entityManager, EntityID self) override {
 *             m_game = game;
 *         }
 * 
 *         void quitTheGame() {
 *             std::cout << "QuitHandlerScript::quitTheGame called, will quit the game" << std::endl;
 *             if (m_game)
 *                 m_game->Quit();
 *         }
 * 
 *         void OnEvent(SDL_Event *event, EntityManager *entityManager, EntityID self) override {
 *             if (event->type == SDL_EVENT_QUIT) {
 *                 quitTheGame();
 *             }
 * 
 *             if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_Q) {
 *                 quitTheGame();
 *             }
 *         }
 *     };
 *     
 *     ent_mgr->addComponentToEntity<QuitHandlerScript>(ent);
 * }
 * ~~~~~~~~~~~~~~~~~~~~~~~~
 */
void InitializeGame(Game*);


#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "pixbench/game.h"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <iostream>
#include <ostream>
#include <string>

// Game* CreateGame();
// void InitializeGame(Game* game);

class AppState {
public:
    Game* game{ nullptr };
    std::string error_message;
};

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    *appstate = new AppState;
    AppState& state = *static_cast<AppState*>(*appstate);

    Game* game = CreateGame();
    if ( !game ) {
        std::cout << "Game::CreateGame return NULL. Quiting now." << std::endl;
        return SDL_APP_FAILURE;
    }

    state.game = game;
    InitializeGame(game);

    std::cout << "App initialization completed" << std::endl;
    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    AppState& state = *static_cast<AppState*>(appstate);

    /* Cascade event */
    Result<VoidResult, GameError> res = state.game->OnEvent(event);
    if ( !res.isOk() ) {
        std::string error_message =
            "SDL_AppEvent=>game::OnEvent Failed with the following error:\n";
        error_message
            .append("\"")
            .append(res.getErrResult()->err_message)
            .append("\"\n");
        state.error_message = error_message;
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

/* This function runs once per frame. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState& state = *static_cast<AppState*>(appstate);

    if (!state.game->isRunning) {
        return SDL_APP_SUCCESS;
    }

    Result<VoidResult, GameError> res = state.game->Itterate();
    if ( !res.isOk() ) {
        std::string error_message =
            "SDL_AppIterate=>game::Itterate Failed with the following error:\n";
        error_message
            .append("\"")
            .append(res.getErrResult()->err_message)
            .append("\"\n");
        state.error_message = error_message;
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    /* SDL will clean up the window/renderer, but deleting game will
     * also explicitly free window and renrerer.*/
    std::cout << "SDL_AppQuit called" << std::endl;
    AppState& state = *static_cast<AppState*>(appstate);

    if (result == SDL_APP_FAILURE && state.game && state.game->renderContext) {
        SDL_ShowSimpleMessageBox(
                SDL_MESSAGEBOX_ERROR,
                "Runtime Error",
                state.error_message.c_str(),
                state.game->renderContext->window
                );
    }

    if ( state.game )
        state.game->OnExit();

    std::cout << "deleting game" << std::endl;
    if ( state.game )
        delete state.game;
    std::cout << "game deleted" << std::endl;
}

#endif
