#include "demo/platformer/include/game_manager.h"
#include "demo/platformer/include/input.h"
#include "demo/platformer/include/player.h"
#include "pixbench/components.h"
#include "pixbench/ecs.h"
#include "pixbench/entity.h"
#include "pixbench/game.h"
#include "pixbench/utils/utils.h"
#include "pixbench/vector2.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_joystick.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_surface.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <ptrauth.h>
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
    gConfig.enable_joystick_and_gamepad = true;
    gConfig.render_clear_color   = Color(100, 100, 100, 255);

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

class InputVisualizer : public CustomRenderable {
private:
    bool is_visible = true;
public:
    Game* m_game;
    InputHandler* inputHandler = nullptr;

    Vector2 m_input = Vector2::ZERO;

    void Init(Game *game, EntityManager *entityManager, EntityID self) override {
        std::cout << "InputVisualizer::Init" << std::endl;
        EntityID inp_handler_ent = entityManager->tag.getEntitiesWithTag("input-handler")[0];
        this->inputHandler = entityManager->getEntityComponent<InputHandler>(inp_handler_ent);
    }

    void drawCross(SDL_Renderer* renderer, Vector2 center, float width) {
        const Vector2 top = center + Vector2::UP * (width / 2.0);
        const Vector2 bottom = center - Vector2::UP * (width / 2.0);
        const Vector2 left = center + Vector2::LEFT * (width / 2.0);
        const Vector2 right = center + Vector2::RIGHT * (width / 2.0);
        SDL_RenderLine(renderer, top.x, top.y, bottom.x, bottom.y);
        SDL_RenderLine(renderer, left.x, left.y, right.x, right.y);
    }

    void drawCircle(SDL_Renderer* renderer, Vector2 center, float radius) {
        const int N = 16;
        SDL_FPoint points[N+1];
        
        for (int i=0; i<=N; ++i) {
            const float angle = std::fmod(((float)i / N) * M_PI * 2, M_PI * 2);
            points[i] = {
                .x = center.x + radius * std::cos(angle),
                .y = center.y + radius * std::sin(angle)
            };
        }
        SDL_RenderLines(renderer, points, N+1);
    }

    void Draw(RenderContext *renderContext, EntityManager *entity_mgr) override {
        int numkeys;
        const bool* kb_state = SDL_GetKeyboardState(&numkeys);
        if (kb_state[SDL_SCANCODE_GRAVE]) {
            is_visible = !is_visible;
        }

        if ( !is_visible )
            return;

        if (inputHandler) {
            m_input.x = inputHandler->getAxisInput("move_x");
            m_input.y = inputHandler->getAxisInput("move_y");
        }

        const Vector2 center = Vector2(300, 300);
        SDL_SetRenderDrawColorFloat(
                renderContext->renderer, 0.0, 0.0, 1.0, 1.0);
        drawCross(renderContext->renderer, center, 120);
        if ( m_input.sqrMagnitude() >= 1.0 ) {
            m_input.normalize();
            SDL_SetRenderDrawColorFloat(
                    renderContext->renderer, 1.0, 0.0, 0.0, 1.0);
        } else {
            SDL_SetRenderDrawColorFloat(
                    renderContext->renderer, 0.0, 0.0, 1.0, 1.0);
        }
        drawCircle(
                renderContext->renderer,
                center, 60);

        SDL_SetRenderDrawColorFloat(
                renderContext->renderer, 1.0, 0.0, 0.0, 1.0);
        drawCircle(
                renderContext->renderer,
                center + 60.0 * Vector2(m_input.x, -m_input.y), 10);

        SDL_RenderDebugTextFormat(
                renderContext->renderer, center.x, center.y - 80.0,
                "m_input: (%.6f, %.6f)", m_input.x, m_input.y
                );
    }
};

void Game::InitializeGame(Game* game) {
    std::cout << "InitializeGame called" << std::endl;
    auto ent_mgr = game->entityManager;

    // create game manager entity
    EntityID game_mgr_ent = ent_mgr->createEntity();
    GameManager* game_mgr = ent_mgr->addComponentToEntity<GameManager>(game_mgr_ent);
    ent_mgr->tag.addTagToEntity(game_mgr_ent, "game-manager");

    // input handler entity
    EntityID input_handler_ent = ent_mgr->createEntity();
    InputHandler* input_handler = ent_mgr->addComponentToEntity<InputHandler>(input_handler_ent);
    ent_mgr->tag.addTagToEntity(input_handler_ent, "input-handler");

    // input visualizer
    EntityID input_vis_ent = ent_mgr->createEntity();
    Transform* input_vis_trans = ent_mgr->addComponentToEntity<Transform>(input_vis_ent);
    InputVisualizer* input_vis = ent_mgr->addComponentToEntity<InputVisualizer>(input_vis_ent);
    input_vis->is_always_visible = true;

    spawnPlayableCharacter(ent_mgr, Vector2(120, 120));

    // spawn simple block bellow
    EntityID block_ent = ent_mgr->createEntity();
    {
        auto transform = ent_mgr->addComponentToEntity<Transform>(block_ent);
        auto box_coll = ent_mgr->addComponentToEntity<BoxCollider>(block_ent);
        box_coll->setSize(200, 20);
        transform->SetPosition(Vector2(120, 200));
    }

    std::cout << "InitializeGame done" << std::endl;
}
