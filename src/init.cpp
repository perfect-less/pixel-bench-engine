#include "pixbench/ecs.h"
#include "pixbench/game.h"
#include "pixbench/physics.h"
#include "pixbench/utils.h"
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
    // gConfig.render_clear_color   = Color(255, 255, 100, 255);
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
        Void Init(Game *game, EntityManager *entityManager, EntityID self) override {
            m_game = game;

            return ResultOK;
        }

        // You can add your own function
        Void quitTheGame() {
            std::cout << "QuitHandlerScript::quitTheGame called, will quit the game" << std::endl;
            if (m_game)
                m_game->Quit();

            return ResultOK;
        }

        Void OnDestroy(EntityManager *entityManager, EntityID self) override {
            std::cout << "QuitHandlerScript::OnDestroy";
            std::cout << " entity->" << self.id << " called to be destroyed";
            std::cout << std::endl;

            return ResultOK;
        }

        // OnEvent called everytime SDL report an event
        Void OnEvent(SDL_Event *event, EntityManager *entityManager, EntityID self) override {
            // Handle quit
            if (event->type == SDL_EVENT_QUIT) {
                quitTheGame();
            }

            if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_Q) {
                quitTheGame();
            }

            // Return a `ResultError` to forcefully terminate the game.
            // this is just an example on how to do it. Normal termination should
            // be using `game->Quit()`
            if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_E) {
                return ResultError(
                        "This is just as simulated error. "
                        "Messages here will be shown on the error pop up."
                        );
            }

            return ResultOK;
        }
    };
    
    // then you can add that component
    ent_mgr->addComponentToEntity<QuitHandlerScript>(ent);

    //
    std::cout << "PHYSICS CREATION BEGIN" << std::endl;

    class BoxController : public ScriptComponent {
    public:
        Game* m_game{ nullptr };
        Vector2 m_input{ Vector2::ZERO };
        int rotation_input{ 0 };

        Result<VoidResult, GameError> Init(Game *game, EntityManager *entityManager, EntityID self) override {
            m_game = game;
            
            return ResultOK;
        }

        Result<VoidResult, GameError> OnEvent(SDL_Event *event, EntityManager *entityManager, EntityID self) override {
            if ( !m_game )
                return ResultOK;

            rotation_input = 0;
            m_input = Vector2::ZERO;
            if (event->type == SDL_EVENT_KEY_DOWN) {
                if (event->key.key == SDLK_A) {
                    m_input.x -= 1.0;
                }
                if (event->key.key == SDLK_D) {
                    m_input.x += 1.0;
                }
                if (event->key.key == SDLK_W) {
                    m_input.y += 1.0;
                }
                if (event->key.key == SDLK_S) {
                    m_input.y -= 1.0;
                }
                
                if (event->key.key == SDLK_X) {
                    rotation_input -= 1;
                }
                if (event->key.key == SDLK_C) {
                    rotation_input += 1;
                }
            }

            if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_L) {
                entityManager->destroyEntity(self);
            }

            return ResultOK;
        }

        Result<VoidResult, GameError> Update(double deltaTime_s, EntityManager *entityManager, EntityID self) override {

            std::cout << "======== FPS : " << 1.0/deltaTime_s << "========" << std::endl;

            Transform* transform = entityManager->getEntityComponent<Transform>(self);
            if ( !transform )
                return ResultOK;
            
            if ( m_input.sqrMagnitude() > 0.0 ) {
                Vector2 last_pos = transform->GlobalPosition();
                Vector2 move_vector = Vector2(m_input.x, -m_input.y) * deltaTime_s * 100.0;
                transform->SetPosition(last_pos + move_vector);
            }

            if ( rotation_input != 0 ) {
                transform->rotation += rotation_input * M_PI * deltaTime_s;
            }
            
            return ResultOK;
        }
    };

    EntityID box1_ent = ent_mgr->createEntity();
    Transform* box1_trans = ent_mgr->addComponentToEntity<Transform>(box1_ent);
    BoxCollider* box1_coll = ent_mgr->addComponentToEntity<BoxCollider>(box1_ent);
    // BoxController* box_cont = ent_mgr->addComponentToEntity<BoxController>(box1_ent);
    box1_trans->SetPosition(Vector2(32.0, 32.0));
    box1_coll->width = 128.0;
    box1_coll->height = 64.0;
    box1_coll->setSize();

    EntityID box2_ent = ent_mgr->createEntity();
    Transform* box2_trans = ent_mgr->addComponentToEntity<Transform>(box2_ent);
    BoxCollider* box2_coll = ent_mgr->addComponentToEntity<BoxCollider>(box2_ent);
    box2_trans->SetPosition(Vector2(128.0, 64.0));
    box2_coll->width = 48.0;
    box2_coll->height = 48.0;
    box2_coll->setSize();

    // polygon - triangles

    Vector2 tris_vertex[3];
    tris_vertex[0] = Vector2(16.0, 16.0);
    tris_vertex[1] = Vector2(0.0, -16.0);
    tris_vertex[2] = Vector2(-16.0, 16.0);
    Polygon poly = Polygon();
    poly.setVertex(tris_vertex, 3);

    EntityID tri_ent = ent_mgr->createEntity();
    Transform* tri_trans = ent_mgr->addComponentToEntity<Transform>(tri_ent);
    PolygonCollider* tri_poly = ent_mgr->addComponentToEntity<PolygonCollider>(tri_ent);
    tri_poly->setPolygon(poly);
    tri_trans->SetPosition(Vector2(128.0, 128.0));
    // BoxController* tri_cont = ent_mgr->addComponentToEntity<BoxController>(tri_ent);



    Vector2 tris_vertex_2[5];
    tris_vertex_2[0] = Vector2(16.0, 16.0);
    tris_vertex_2[1] = Vector2(16.0, -16.0);
    tris_vertex_2[2] = Vector2(0.0, -32.0);
    tris_vertex_2[3] = Vector2(-16.0, -16.0);
    tris_vertex_2[4] = Vector2(-16.0, 16.0);
    Polygon poly_2 = Polygon();
    poly_2.setVertex(tris_vertex_2, 5);

    EntityID tri_ent_2 = ent_mgr->createEntity();
    Transform* tri_trans_2 = ent_mgr->addComponentToEntity<Transform>(tri_ent_2);
    PolygonCollider* tri_poly_2 = ent_mgr->addComponentToEntity<PolygonCollider>(tri_ent_2);
    tri_poly_2->setPolygon(poly_2);
    tri_trans_2->SetPosition(Vector2(128.0 + 64.0, 128.0));


    EntityID circ_ent = ent_mgr->createEntity();
    Transform* circ_trans = ent_mgr->addComponentToEntity<Transform>(circ_ent);
    CircleCollider* circ_coll = ent_mgr->addComponentToEntity<CircleCollider>(circ_ent);
    circ_coll->radius = 32.0;
    tri_trans_2->SetPosition(Vector2(128.0 + 64.0, 128.0));
    BoxController* circ_cont = ent_mgr->addComponentToEntity<BoxController>(circ_ent);

    std::cout << "PHYSICS CREATION DONE" << std::endl;
    //

    // destroying entity
    // ent_mgr->destroyEntity(ent);

    std::cout << "InitializeGame done" << std::endl;
}
