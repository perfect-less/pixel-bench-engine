#pragma once

#include "demo/platformer/include/input.h"
#include "pixbench/components.h"
#include "pixbench/ecs.h"
#include "pixbench/entity.h"
#include "pixbench/physics/type.h"
#include "pixbench/vector2.h"


EntityID spawnPlayableCharacter(EntityManager* ent_mgr, Vector2 spawn_position=Vector2::ZERO);


class CharacterController : public ScriptComponent {
private:
    Game* game = nullptr;
    InputHandler* input_handler = nullptr;

    Transform* transform = nullptr;
    CapsuleCollider* caps_coll = nullptr;

    Vector2 m_move_input = Vector2::ZERO;

    Vector2 velocity = Vector2::ZERO;
    
public:
    
    double gravity_multiplier = 10.0;

    void onCollisionEnter(CollisionEvent coll_event);

    void onCollisionLeave(EntityID other);

    float walk_speed = 100.0;
    float run_speed = 200.0;

    Result<VoidResult, GameError> Init(Game *game, EntityManager *entityManager, EntityID self) override;

    Result<VoidResult, GameError> Update(double deltaTime_s, EntityManager *entityManager, EntityID self) override;

    Result<VoidResult, GameError> FixedUpdate(double deltaTime_s, EntityManager *entityManager, EntityID self) override;
};
