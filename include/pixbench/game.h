#ifndef GAME_HEADER
#define GAME_HEADER


#include "pixbench/engine_config.h"
#include "pixbench/gameconfig.h"
#include "pixbench/renderer.h"
#include "pixbench/audio.h"
#include "pixbench/utils.h"
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


/*
 * Default error type to return using Result<T, E> by Game class
 * methods.
 * example usage:
 * ~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * return Result<VoidResult, GameError>::Err(GameError("Error message here."))
 * ~~~~~~~~~~~~~~~~~~~~~~~~
 */
class GameError {
public:
    std::string err_message;

    GameError() = default;

    GameError(
            std::string err_message
            )
        :
            err_message(err_message)
    { }
};

/*
 * Place holder to denote `void` type of return when using `Result<T, E>`
 * example usage:
 * ~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * return Result<VoidResult, GameError>::Ok(VoidResult::empty)
 * ~~~~~~~~~~~~~~~~~~~~~~~~
 */
class VoidResult {
public:
    static VoidResult empty;
};


/*
 * Default Void Result return value used by the game engine.
 */
#define Void Result<VoidResult, GameError>

/*
 * Default Ok value for typical Result<VoidResult, GameError> result type used
 * by the game engine.
 */
#define ResultOK Result<VoidResult, GameError>::Ok(VoidResult::empty)

/*
 * Default Error value for typical Result<VoidResult, GameError> result type
 * used by the game engine.
 */
#define ResultError(err_message) Result<VoidResult, GameError>::Err(GameError(err_message))


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
     * **User defined function** that sould be overriden. This function
     * shall be used to create a Game object and calling the Game::Initialize()
     * function.
     * Example of the usage of this function
     * ~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
     * Game* Game::CreateGame() {
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
    static Game* CreateGame();

    /**
     * **User defined function** that should be overriden by the user. You can
     * loads assets and then creates your entities here.
     * example usage: 
     * ~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
     * void Game::InitializeGame(Game* game) {
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
    static void InitializeGame(Game*);

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
     * Read more on: `EntityManager::setOnEntityDestroyedCallback`
     */
    void OnComponentRegistered(ComponentDataPayload component_payload);

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
