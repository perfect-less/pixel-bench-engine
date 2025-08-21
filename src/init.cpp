#include "pixbench/ecs.h"
#include "pixbench/entity.h"
#include "pixbench/game.h"
#include "pixbench/physics.h"
#include "pixbench/renderer.h"
#include "pixbench/resource.h"
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

    class RayDrawer : public CustomRenderable {
    public:
        double radius = 0.0;
        Vector2 ray_origin = Vector2::ZERO;
        Vector2 ray_endpoint = Vector2::ZERO;
        bool is_drawing = false;
        bool is_hitting = false;
        Vector2 hit_point;
        Vector2 hit_normal;

        void setRayState(Vector2 origin, Vector2 end, bool is_drawing=true) {
            this->ray_origin = origin;
            this->ray_endpoint = end;
            this->is_drawing = is_drawing;
        }

        void resetHit() {
            this->is_hitting = false;
        }
        void setHitPoint(Vector2 hit_point, Vector2 hit_normal) {
            this->hit_point = hit_point;
            this->hit_normal = hit_normal;
            this->is_hitting = true;
        }

        void Draw(RenderContext *renderContext, EntityManager *entity_mgr) override {
            if ( !is_drawing )
                return;

            // std::cout << "====================================================================================================DRAWING RAYS" << std::endl;
            SDL_SetRenderDrawColorFloat(renderContext->renderer, 1.0, 0.0, 0.0, 1.0);
            const Vector2 _origin = sceneToScreenSpace(
                    renderContext, this->ray_origin
                    );
            Vector2 _end = sceneToScreenSpace(
                    renderContext, this->ray_endpoint
                    );
            if (radius != 0.0) {
                _end += radius * hit_normal;
            } 
            SDL_RenderLine(renderContext->renderer, _origin.x, _origin.y, _end.x, _end.y);

            if ( is_hitting ) {
                const double line_length = 30.0;
                const Vector2 _hit_point = sceneToScreenSpace(renderContext, hit_point);
                const Vector2 hit_line_end = _hit_point + hit_normal * line_length;
                SDL_RenderLine(
                        renderContext->renderer,
                        _hit_point.x, _hit_point.y, hit_line_end.x, hit_line_end.y
                        );
            }
        }
    };

    class BoxController : public ScriptComponent {
    public:
        Game* m_game{ nullptr };
        RayDrawer* ray_drawer{ nullptr };
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

            // if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_L) {
            //     entityManager->destroyEntity(self);
            // }

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

            RayDrawer* ray_drawer = entityManager->getEntityComponent<RayDrawer>(self);
            if ( !ray_drawer ) {
                std::cout << ">>>>> ray_drawer IS NULL <<<<<" << std::endl;
                return ResultOK;
            }

            RaycastHit out__raycast_hit;
            float ray_length = 400.0;
            Vector2 ray_origin = transform->GlobalPosition();
            Vector2 ray_direction = Vector2::UP.rotated(transform->rotation);
            Vector2 ray_end = ray_origin + ray_direction * ray_length;
            // const bool is_hit = m_game->physics.rayCast(
            //         ray_origin, ray_direction,
            //         ray_length, &out__raycast_hit
            //         );
            const bool is_hit = m_game->physics.circleCast(
                    ray_origin, ray_direction,
                    ray_length, 10.0, &out__raycast_hit
                    );
            ray_drawer->radius = 10.0;

            ray_drawer->resetHit();
            if ( is_hit ) {
                std::cout << "==================== HITTING: " << out__raycast_hit.ent.id << std::endl;
                ray_end = out__raycast_hit.point;
                ray_drawer->setHitPoint(out__raycast_hit.point, out__raycast_hit.normal);
            }
            
            ray_drawer->setRayState(ray_origin, ray_end);
            
            return ResultOK;
        }
    };

    class RotatingBody : public ScriptComponent {
    public:

        float rotation_speed = 0.0;

        Result<VoidResult, GameError> Update(double deltaTime_s, EntityManager *entityManager, EntityID self) override {
            Transform* transform = entityManager->getEntityComponent<Transform>(self);
            if ( !transform )
                return ResultOK;

            transform->rotation += deltaTime_s * rotation_speed;

            return ResultOK;
        }
    };

    class CameraController : public ScriptComponent {
    public:
        float move_speed = 128.0;
        bool kstate[4] = {false, false, false, false};
        
        Result<VoidResult, GameError> OnEvent(SDL_Event *event, EntityManager *entityManager, EntityID self) override {
            if (event->type == SDL_EVENT_KEY_DOWN) {
                if (event->key.key == SDLK_L) {
                    kstate[1] = true;
                }
                if (event->key.key == SDLK_J) {
                    kstate[0] = true;
                }
                if (event->key.key == SDLK_K) {
                    kstate[2] = true;
                }
                if (event->key.key == SDLK_I) {
                    kstate[3] = true;
                }
            }
            if (event->type == SDL_EVENT_KEY_UP) {
                if (event->key.key == SDLK_L) {
                    kstate[1] = false;
                }
                if (event->key.key == SDLK_J) {
                    kstate[0] = false;
                }
                if (event->key.key == SDLK_K) {
                    kstate[2] = false;
                }
                if (event->key.key == SDLK_I) {
                    kstate[3] = false;
                }
            }
            return ResultOK;
        }

        Game* m_game = nullptr;
        Result<VoidResult, GameError> Init(Game *game, EntityManager *entityManager, EntityID self) override {
            m_game = game;
            // m_game->renderContext->SetCameraContext(
            //         m_game->renderContext->camera_position,
            //         Vector2(320, 240));
            return ResultOK;
        }

        Result<VoidResult, GameError> LateUpdate(double deltaTime_s, EntityManager *entityManager, EntityID self) override {
            float x_input = kstate[1] - kstate[0];
            float y_input = kstate[2] - kstate[3];
            float cam_x_pos = m_game->renderContext->camera_position.x;
            float cam_y_pos = m_game->renderContext->camera_position.y;
            cam_x_pos += x_input * move_speed * deltaTime_s;
            cam_y_pos += y_input * move_speed * deltaTime_s;
            m_game->renderContext->SetCameraContext(
                    Vector2(cam_x_pos, cam_y_pos),
                    m_game->renderContext->camera_size
                    );
            std::cout << std::endl;
            std::cout << "CAM X POS: " << m_game->renderContext->camera_position.x << std::endl;
            std::cout << "CAM X SIZE: " << m_game->renderContext->camera_size.x << std::endl;
            std::cout << std::endl;
            return ResultOK;
        }
    };
    
    EntityID cam_cont_ent = ent_mgr->createEntity();
    ent_mgr->addComponentToEntity<CameraController>(cam_cont_ent);

    // sprite anchor
    auto box_tex = LoadSDLTexture("assets/box.bmp", game->renderContext->renderer);
    
    EntityID box_ent = ent_mgr->createEntity();
    Transform* box_trans = ent_mgr->addComponentToEntity<Transform>(box_ent);
    Sprite* box_sprite = ent_mgr->addComponentToEntity<Sprite>(box_ent);
    box_sprite->SetTexture(box_tex);
    box_trans->SetPosition(Vector2(32.0, 32.0));


    EntityID box1_ent = ent_mgr->createEntity();
    Transform* box1_trans = ent_mgr->addComponentToEntity<Transform>(box1_ent);
    BoxCollider* box1_coll = ent_mgr->addComponentToEntity<BoxCollider>(box1_ent);
    // BoxController* box_cont = ent_mgr->addComponentToEntity<BoxController>(box1_ent);
    box1_trans->SetPosition(Vector2(32.0, 32.0));
    box1_trans->rotation = 0.001;
    box1_coll->setSize(128.0, 64.0);

    EntityID box2_ent = ent_mgr->createEntity();
    Transform* box2_trans = ent_mgr->addComponentToEntity<Transform>(box2_ent);
    BoxCollider* box2_coll = ent_mgr->addComponentToEntity<BoxCollider>(box2_ent);
    RotatingBody* box2_rotator = ent_mgr->addComponentToEntity<RotatingBody>(box2_ent);
    box2_rotator->rotation_speed = M_PI / 2.0;
    box2_trans->SetPosition(Vector2(128.0, 64.0));
    box2_trans->rotation = 10.0;
    box2_coll->setSize(48.0, 48.0);
    box2_coll->is_static = false;

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
    tri_trans->rotation = M_PI/4.0;
    BoxController* tri_cont = ent_mgr->addComponentToEntity<BoxController>(tri_ent);
    ent_mgr->addComponentToEntity<RayDrawer>(tri_ent);
    tri_poly->is_static = false;



    Vector2 tris_vertex_2[6];
    tris_vertex_2[0] = Vector2(32.0, 32.0);
    tris_vertex_2[1] = Vector2(32.0, -32.0);
    tris_vertex_2[2] = Vector2(0.0, -64.0);
    tris_vertex_2[3] = Vector2(-32.0, -32.0);
    tris_vertex_2[4] = Vector2(-32.0, 32.0);
    tris_vertex_2[5] = Vector2(0.0, 64.0);
    Polygon poly_2 = Polygon();
    poly_2.setVertex(tris_vertex_2, 6);

    EntityID tri_ent_2 = ent_mgr->createEntity();
    Transform* tri_trans_2 = ent_mgr->addComponentToEntity<Transform>(tri_ent_2);
    PolygonCollider* tri_poly_2 = ent_mgr->addComponentToEntity<PolygonCollider>(tri_ent_2);
    tri_poly_2->setPolygon(poly_2);
    tri_trans_2->SetPosition(Vector2(128.0 + 64.0, 128.0));
    tri_trans_2->rotation = (45.0/180.0)*M_PI;

    
    EntityID circ_ent = ent_mgr->createEntity();
    Transform* circ_trans = ent_mgr->addComponentToEntity<Transform>(circ_ent);
    CircleCollider* circ_coll = ent_mgr->addComponentToEntity<CircleCollider>(circ_ent);
    circ_coll->setRadius(80.0);
    circ_trans->SetPosition(Vector2(300.0, 300.0));
    

    std::cout << "PHYSICS CREATION DONE" << std::endl;
    //

    // destroying entity
    // ent_mgr->destroyEntity(ent);

    std::cout << "InitializeGame done" << std::endl;
}
