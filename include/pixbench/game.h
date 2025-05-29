#ifndef GAME_HEADER
#define GAME_HEADER


#include "pixbench/gameconfig.h"
#include "pixbench/renderer.h"
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>
#include <memory>
#include <string>
#include <vector>


struct ComponentDataPayload;
class EntityManager;
class ISystem;
class ScriptSystem;


class Game {
private:
    GameConfig gameConfig;
    std::string basePath;
public:
    const char* title;
    RenderContext* renderContext;
    EntityManager* entityManager = nullptr;

    std::shared_ptr<ISystem> scriptSystem = nullptr;
    std::vector<std::shared_ptr<ISystem>> ecs_systems;

    double lastTicksNS = 0;
    bool isRunning = false;
    
    Game ();
    ~Game();

    static Game* CreateGame();
    static void InitializeGame(Game*);

    void ApplyGameConfig(GameConfig newConfig);

    void PrepareUtils();
    void PrepareRenderer(int windowWidth, int windowHeight);

    void Initialize();

    void Itterate();
    void OnComponentRegistered(ComponentDataPayload component_payload);
    void OnEvent(SDL_Event *event);
    void Quit();
    void OnError();
    void OnExit();

    std::string& GetBasePath() {return basePath;};
};


#endif
