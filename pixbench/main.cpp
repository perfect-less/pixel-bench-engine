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
#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

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

    Game* game = Game::CreateGame();
    if ( !game ) {
        std::cout << "Game::CreateGame return NULL. Quiting now." << std::endl;
        return SDL_APP_FAILURE;
    }

    state.game = game;
    game->InitializeGame(game);

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
