#include "pixbench/game.h"
#include "pixbench/ecs.h"
#include "pixbench/systems.h"
#include "pixbench/engine_config.h"
#include "pixbench/vector2.h"
#include "SDL3_mixer/SDL_mixer.h"
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

Result<VoidResult, GameError> Game::Initialize() {
    auto res = PrepareUtils();
    if ( !res.isOk() )
        return res;

    bool is_success = SDL_SetAppMetadata(
            this->gameConfig.game_title.c_str(),
            this->gameConfig.game_version.c_str(),
            this->gameConfig.game_identifier.c_str()
            );
    if ( !is_success )
        return Result<VoidResult, GameError>::Err(
                GameError(
                    "Can't set app metadata with SDL_SetAppMetadata. "
                    "Perhaps there's a problem with the SDL installation"
                    )
                );

    // Initialize SDL
    SDL_InitFlags init_flag = SDL_INIT_VIDEO | SDL_INIT_AUDIO;
    if (this->gameConfig.enable_joystick_and_gamepad) 
        init_flag |= SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD;
    if (!SDL_Init(init_flag)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Can't initialized SDL: %s`", SDL_GetError());
        return Result<VoidResult, GameError>::Err(
                GameError("Can't initialize SDL")
                );
    }

    res = PrepareRenderer(
            this->gameConfig.window_width,
            this->gameConfig.window_height
            );
    if ( !res.isOk() )
        return res;

    
    res = PrepareAudio();
    if ( !res.isOk() )
        return res;

    this->entityManager = new EntityManager();
    this->entityManager->setComponentAddedToEntityCallback(
            [this] (ComponentTag ctag, ComponentType ctype, size_t cindex, EntityID ent_id)
            {
            ComponentDataPayload payload;
            payload.ctag = ctag;
            payload.ctype = ctype;
            payload.cindex = cindex;
            this->OnComponentAddedToEntity(payload, ent_id);
            }
            );
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
    this->physicsSystem = std::make_shared<PhysicsSystem>();
    this->ecs_systems.push_back(std::make_shared<RenderingSystem>());
    this->ecs_systems.push_back(std::make_shared<AudioSystem>());
    this->ecs_systems.push_back(physicsSystem);
    this->ecs_systems.push_back(scriptSystem);

    this->isRunning = true;

    // Physics
    this->physics.__setGame(this);

    return Result<VoidResult, GameError>::Ok(VoidResult::empty);
}

Result<VoidResult, GameError> Game:: PrepareUtils() {
    // Prepare for random number generator
    // PrepareRandomGenerator();

    // Base path of the executable
    const char* base_path_cstr = SDL_GetBasePath();
    if ( !base_path_cstr )
        return Result<VoidResult, GameError>::Err(
                GameError("Can't obtain executable base path using SDL_GetBasePath")
                );

    this->basePath = base_path_cstr;

    return Result<VoidResult, GameError>::Ok(VoidResult::empty);
}


Result<VoidResult, GameError> Game::PrepareRenderer(int windowWidth, int windowHeight) {

    if (renderContext) {
        /* Should fail as renderContext has been initialized */
        return Result<VoidResult, GameError>::Err(
                GameError("PrepareRenderer() failed, renderContext already initialized")
                );
    }
    this->renderContext = new RenderContext(
            this->gameConfig.game_title.c_str(),
            Vector2(windowWidth, windowHeight),
            Vector2(0.0, 0.0),
            Vector2(windowWidth, windowHeight),
            this->gameConfig.render_clear_color
            );

    if (this->gameConfig.render_vsync_enabled) {
        if (!SDL_SetRenderVSync(this->renderContext->renderer, 1)) {
            SDL_Log("Couldn't enable VSync.");
            Result<VoidResult, GameError>::Err(
                    GameError("Couldn't enable VSync with SDL.")
                    );
        }
    }

    return Result<VoidResult, GameError>::Ok(VoidResult::empty);
}


Void Game::PrepareAudio() {
    // Create audioContext object
    this->audioContext = new AudioContext(
            AUDIO_NUM_CHANNELS
            );

    // Initalize SDL_mixer
    MIX_InitFlags mix_init_res = Mix_Init(AUDIO_MIX_INIT_FLAGS);
    if ( !mix_init_res ) {
        std::string err_message = 
            "Can't initialize SDL_Mixer: ";
        err_message.append(SDL_GetError());
        return Result<VoidResult, GameError>::Err(
                GameError(err_message)
                );
    }

    return ResultOK;
}


void Game::OnComponentRegistered(ComponentDataPayload component_payload) {
    for (auto& system : this->ecs_systems) {
        system->OnComponentRegistered(&component_payload);
    }
}


void Game::OnComponentAddedToEntity(ComponentDataPayload component_payload, EntityID entity_id) {
    for (auto& system : this->ecs_systems) {
        system->OnComponentAddedToEntity(&component_payload, entity_id);
    }
}


void Game::OnEntityDestroyed(EntityID entity_id) {
    for (auto& system : this->ecs_systems) {
        system->OnEntityDestroyed(this->entityManager, entity_id);
    }
}


Result<VoidResult, GameError> Game::OnEvent(SDL_Event *event) {
    std::cout << "Game::OnEvent called" << std::endl;
    // if (event->type == SDL_EVENT_QUIT) {
    //     // return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    //     this->Quit();
    // }

    double lastTicks = this->lastTicksU__ns;

    // Let the system handle the event
    for (auto& system : this->ecs_systems) {
        auto res = system->OnEvent(event, this->entityManager);
        if ( !res.isOk() )
            return res;
    }

    return ResultOK;
}

Result<VoidResult, GameError> Game::Itterate() {
    double now_ns = ((double)SDL_GetTicksNS());
    double delta_time_s = (now_ns - this->lastTicksU__ns) / 1000000000.0;

    // std::cout << "Game::Itterate called (" << delta_time_s << ")" << std::endl;

    Result<VoidResult, GameError> res;

    // TO DO: Init cascade
    for (auto& system : this->ecs_systems) {
        res = system->Initialize(this, this->entityManager);
        if ( !res.isOk() ) {
            return res;
        }
    }
    this->entityManager->resetEntitiesUninitializedStatus();
    

    // TO DO: Update cascade
    for (auto& system : this->ecs_systems) {
        res = system->Update(delta_time_s, this->entityManager);
        if ( !res.isOk() ) {
            return res;
        }
    }
    this->lastTicksU__ns = now_ns;

    // TO DO: FixedUpdate cascade, `while update is due`
    // while (not is_deadline_been_made()) {
    //      FixedUpdate();
    // }
    while ( this->lastTicksFU__ns < now_ns ) {
        delta_time_s = 1.0/((double)FIXED_UPDATE_RATE);

        for (auto& system : this->ecs_systems) {
            res = system->FixedUpdate(delta_time_s, this->entityManager);
            if ( !res.isOk() ) {
                return res;
            }
        }

        now_ns = ((double)SDL_GetTicksNS());
        this->lastTicksFU__ns += delta_time_s*1000000000.0;
    }

    // TO DO: LateUpdate cascade
    now_ns = ((double)SDL_GetTicksNS());
    delta_time_s = (now_ns - this->lastTicksLU__ns) / 1000000000.0;
    for (auto& system : this->ecs_systems) {
        res = system->LateUpdate(delta_time_s, this->entityManager);
        if ( !res.isOk() ) {
            return res;
        }
    }
    this->lastTicksLU__ns = now_ns;


    bool is_success;

    // Clear screen before render
    is_success = SDL_SetRenderDrawColor(
            this->renderContext->renderer,
            this->renderContext->renderClearColor.r,
            this->renderContext->renderClearColor.g,
            this->renderContext->renderClearColor.b,
            this->renderContext->renderClearColor.a
            );
    if ( !is_success ) {
        std::string err_msg =
            "Can't set base color to screen. SDL_SetRenderDrawColor failed:\n";
        err_msg.append(SDL_GetError());
        return Result<VoidResult, GameError>::Err(
                GameError(err_msg)
                );
    }

    is_success = SDL_RenderClear(this->renderContext->renderer);
    if ( !is_success ) {
        std::string err_msg =
            "Can't draw base color to screen. SDL_RenderClear failed:\n";
        err_msg.append(SDL_GetError());
        return Result<VoidResult, GameError>::Err(
                GameError(err_msg)
                );
    }

    // TO DO: PreDraw Calls
    for (auto& system : this->ecs_systems) {
        res = system->PreDraw(
                this->renderContext,
                this->entityManager
                );
        if ( !res.isOk() ) {
            return res;
        }
    }

    // TO DO: Render ordering based on depth value (Int32)
    // TO DO: Draw Calls (draw from back to front, largest depth value to smallest)
    for (auto& system : this->ecs_systems) {
        res = system->Draw(
                this->renderContext,
                this->entityManager
                );
        if ( !res.isOk() ) {
            return res;
        }
    }

    // Screen Update (flip the screen)
    is_success = SDL_RenderPresent(this->renderContext->renderer);
    if ( !is_success ) {
        std::string err_message =
            "Can't update the screen, SDL_RenderPresent failed:\n";
        err_message.append(SDL_GetError());
        return Result<VoidResult, GameError>::Err(
                err_message
                );
    }


    return ResultOK;
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
