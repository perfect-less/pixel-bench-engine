#ifndef GAME_HEADER
#define GAME_HEADER


#include "pixbench/gameconfig.h"
#include "pixbench/hierarchy.h"
#include "pixbench/physics/physics.h"
#include "pixbench/renderer.h"
#include "pixbench/audio.h"
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>
#include <memory>
#include <string>
#include <vector>


struct ComponentDataPayload;
struct EntityID;
class EntityManager;
class ISystem;
class ScriptSystem;


/**
 * Game object contains all contexts and manages everything related to the
 * running of the game. Game object also the one that pass SDL event and
 * callbacks to game entities and systems.
 */
class Game {
private:
    //!< config for the game
    GameConfig gameConfig;
    //!< basepath of the executable
    std::string basePath;
public:
    const char* title;                          //!< game title
    RenderContext* renderContext;               //!< global render context
    AudioContext* audioContext;                 //!< global audio context
    EntityManager* entityManager = nullptr;     //!< global entityManager, created when Game::Initialize() were called

    PhysicsAPI physics;
    HierarchyAPI entityHierarchy;

    std::shared_ptr<ISystem> hierarchySystem = nullptr; //!< hierarchy system
    std::shared_ptr<ISystem> physicsSystem = nullptr;
    std::shared_ptr<ISystem> scriptSystem = nullptr;    //!< script system
    std::vector<std::shared_ptr<ISystem>> ecs_systems;  //!< array of systems in ECS

    double lastTicksU__ns = 0;                  //!< used to keep tracks of game Update ticks
    double lastTicksFU__ns = 0;                 //!< used to keep tracks of game FixedUpdate ticks
    double lastTicksLU__ns = 0;                 //!< used to keep tracks of game LateUpdate ticks
    bool isRunning = false;                     //!< Reflecting the status of whether the game is currently running or not
    
    /**
     * Game constructor, usage:
     * ~~~~~~~~~~~~~~~~~~~~{.cpp}
     * Game* game = new Game();
     * ~~~~~~~~~~~~~~~~~~~~
     */
    Game ();
    ~Game();

    /**
     * Apply game config created using GameConfig class. Only called this
     * inside Game::CreateGame() function
     */
    void ApplyGameConfig(GameConfig newConfig);

    /**
     * Called by Game::Initialize()
     * Initialized required utilities used by Game object 
     * (e.g. random number generator, geting base path, etc.)
     */
    Result<VoidResult, GameError> PrepareUtils();

    /**
     * Create renderContext and apply configs (e.g vsync)
     */
    Result<VoidResult, GameError> PrepareRenderer(int windowWidth, int windowHeight);

    /**
     * Create audioContext and apply configs (e.g frequency, etc.)
     */
    Result<VoidResult, GameError> PrepareAudio();

    /**
     * Initialize a Game
     * This function will initialize SDL, create window, setting up renderer, etc.
     * 
     * **Note**: *don't forget to call this function in Game::CreateGame() that you've 
     * defined.*
     */
    Result<VoidResult, GameError> Initialize();

    /**
     * Called by SDL_AppIterate callback
     * This function will call Init, Update, LateUpdate, FixedUpdate, PreDraw,
     * and Draw to all registered systems that implement it.
     */
    Result<VoidResult, GameError> Itterate();

    /**
     * Callback called when a new Component type is registered to ComponentManager
     *
     * Read more on: `EntityManager::setComponentRegisterCallback`
     */
    void OnComponentRegistered(ComponentDataPayload component_payload);

    /**
     * Callback called when a Component is added to an entity
     *
     * Read more on: `EntityManager::setComponentAddedToEntityCallback`
     */
    void OnComponentAddedToEntity(ComponentDataPayload component_payload, EntityID entity_id);

    /**
     * Callback called when an entity will be destroyed.
     *
     */
    void OnEntityDestroyed(EntityID entity_id);

    /**
     * Called by SDL_AppEvent callback to pass down event to all registered systems.
     */
    Result<VoidResult, GameError> OnEvent(SDL_Event *event);

    /**
     * Quit the game
     */
    void Quit();

    void OnError();
    void OnExit();

    /**
     * Return base_path of the game executable.
     */
    std::string& GetBasePath() {return basePath;};
};


#endif
