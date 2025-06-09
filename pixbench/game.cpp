#include "pixbench/game.h"
#include "pixbench/ecs.h"
#include "pixbench/utils.h"
#include "pixbench/vector2.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <iostream>
#include <memory>
#include <string>


Game::Game () 
    : 
    renderContext(nullptr),
    isRunning(false)
{
}

Game::~Game () {
    if (this->renderContext) {
        delete this->renderContext;
    }
}

void Game::ApplyGameConfig (GameConfig newConfig) {
    this->gameConfig = newConfig;
}

void Game::Initialize() {
    PrepareUtils();

    SDL_SetAppMetadata(
            this->gameConfig.game_title.c_str(),
            this->gameConfig.game_version.c_str(),
            this->gameConfig.game_identifier.c_str()
            );

    // Initialize SDL
    SDL_InitFlags init_flag = SDL_INIT_VIDEO;
    if (this->gameConfig.enable_joystick_and_gamepad) 
        init_flag |= SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD;
    if (!SDL_Init(init_flag)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Can't initialized SDL: %s`", SDL_GetError());
    }

    PrepareRenderer(
            this->gameConfig.window_width,
            this->gameConfig.window_height
            );

    this->entityManager = new EntityManager();
    this->entityManager->setComponentRegisterCallback(
            [this] (ComponentTag ctag, ComponentType ctype, size_t cindex) 
            {
            ComponentDataPayload payload;
            payload.ctag = ctag;
            payload.ctype = ctype;
            payload.cindex = cindex;
            this->OnComponentRegistered(payload);
            }
            );
    this->entityManager->setOnEntityDestroyedCallback(
            [this] (EntityID entity_id)
            {
            this->OnEntityDestroyed(entity_id);
            }
            );

    this->scriptSystem = std::make_shared<ScriptSystem>();
    this->ecs_systems.push_back(std::make_shared<RenderingSystem>());
    this->ecs_systems.push_back(scriptSystem);

    this->isRunning = true;
}

void Game:: PrepareUtils() {
    // Prepare for random number generator
    PrepareRandomGenerator();

    // Base path of the executable
    this->basePath = SDL_GetBasePath();
}


void Game::PrepareRenderer(int windowWidth, int windowHeight) {

    if (renderContext) {

        std::cout << "renderContext already initialized" << std::endl;
        /* Should fail as renderContext has been initialized */
    }
    this->renderContext = new RenderContext(
            this->gameConfig.game_title.c_str(),
            Vector2(0.0, 0.0),
            Vector2(windowWidth, windowHeight),
            this->gameConfig.render_clear_color
            );

    if (this->gameConfig.render_vsync_enabled) {
        if (!SDL_SetRenderVSync(this->renderContext->renderer, 1)) {
            SDL_Log("Couln't enable VSync.");
        }
    }
}


void Game::OnComponentRegistered(ComponentDataPayload component_payload) {
    for (auto& system : this->ecs_systems) {
        system->OnComponentRegistered(&component_payload);
    }
}


void Game::OnEntityDestroyed(EntityID entity_id) {
    for (auto& system : this->ecs_systems) {
        system->OnEntityDestroyed(this->entityManager, entity_id);
    }
}


void Game::OnEvent(SDL_Event *event) {
    std::cout << "Game::OnEvent called" << std::endl;
    // if (event->type == SDL_EVENT_QUIT) {
    //     // return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    //     this->Quit();
    // }

    double lastTicks = this->lastTicksNS;

    // Let the system handle the event
    for (auto& system : this->ecs_systems) {
        system->OnEvent(event, this->entityManager);
    }
}

void Game::Itterate() {
    double now_ns = ((double)SDL_GetTicksNS());
    double delta_time_s = (now_ns - this->lastTicksNS) / 1000000000.0;

    // std::cout << "Game::Itterate called (" << delta_time_s << ")" << std::endl;

    // TO DO: Init cascade
    for (auto& system : this->ecs_systems) {
        system->Initialize(this, this->entityManager);
    }
    this->entityManager->resetEntitiesUninitializedStatus();
    

    // TO DO: Update cascade
    for (auto& system : this->ecs_systems) {
        system->Update(delta_time_s, this->entityManager);
    }

    // TO DO: FixedUpdate cascade, `while update is due`
    for (auto& system : this->ecs_systems) {
        system->FixedUpdate(delta_time_s, this->entityManager);
    }

    // TO DO: LateUpdate cascade
    now_ns = ((double)SDL_GetTicksNS());
    delta_time_s = (now_ns - this->lastTicksNS) / 1000000000.0;
    for (auto& system : this->ecs_systems) {
        system->LateUpdate(delta_time_s, this->entityManager);
    }

    // TO DO: Handle nodes destruction

    // Clear screen before render
    SDL_SetRenderDrawColor(
            this->renderContext->renderer,
            this->renderContext->renderClearColor.r,
            this->renderContext->renderClearColor.g,
            this->renderContext->renderClearColor.b,
            this->renderContext->renderClearColor.a
            );
    SDL_RenderClear(this->renderContext->renderer);

    // TO DO: PreDraw Calls
    for (auto& system : this->ecs_systems) {
        system->PreDraw(
                this->renderContext,
                this->entityManager
                );
    }

    // TO DO: Render ordering based on depth value (Int32)
    // TO DO: Draw Calls (draw from back to front, largest depth value to smallest)
    for (auto& system : this->ecs_systems) {
        system->Draw(
                this->renderContext,
                this->entityManager
                );
    }

    // Screen Update (flip the screen)
    SDL_RenderPresent(this->renderContext->renderer);

    this->lastTicksNS = now_ns;
}


void Game::Quit () {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Game::Quit Called");
    this->isRunning = false;
}


void Game::OnExit() {
    // destroy all entities (along it's components)
    this->entityManager->destroyAllEntities();

    // destroy all systems
    for (auto& system : this->ecs_systems) {
        system->OnExit(this->entityManager);
    }
    
    // SDL_window and SDL_renderere will be handled by renderContext
}


void Game::OnError() {
    // TODO: log error
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "An Error has occured"); // TEMPORARY
    // exit the game
    this->OnExit();
}
