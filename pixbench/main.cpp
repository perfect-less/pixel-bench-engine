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
#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

class AppState {
public:
    Game* game;
};

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    *appstate = new AppState;
    AppState& state = *static_cast<AppState*>(*appstate);

    /*SDL_SetAppMetadata(G_game_title, G_game_version, G_game_identifier);*/

    Game* game = Game::CreateGame();
    /*Game* game = new Game(G_game_title, G_window_width, G_window_height);*/
    state.game = game;

    game->InitializeGame(game);

    std::cout << "App initialization completed" << std::endl;
    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    AppState& state = *static_cast<AppState*>(appstate);
    // state.game->OnEvent(event);

    /* Cascade event */
    state.game->OnEvent(event);

    return SDL_APP_CONTINUE;
}

/* This function runs once per frame. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState& state = *static_cast<AppState*>(appstate);

    if (!state.game->isRunning) {
        return SDL_APP_SUCCESS;
    }

    state.game->Itterate();

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    /* SDL will clean up the window/renderer, but deleting game will
     * also explicitly free window and renrerer.*/
    std::cout << "SDL_AppQuit called" << std::endl;
    AppState& state = *static_cast<AppState*>(appstate);

    state.game->OnExit();

    std::cout << "deleting game" << std::endl;
    delete state.game;
    std::cout << "game deleted" << std::endl;
}
