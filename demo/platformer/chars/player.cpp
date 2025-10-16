#include "demo/platformer/include/player.h"
#include "demo/platformer/include/input.h"
#include "pixbench/components.h"
#include "pixbench/ecs.h"
#include "pixbench/entity.h"
#include "pixbench/physics/type.h"
#include "pixbench/utils/results.h"
#include "pixbench/vector2.h"


EntityID spawnPlayableCharacter(EntityManager *ent_mgr, Vector2 spawn_position) {

    std::cout << "==================== Character Spawn Called ====================" << std::endl;

    EntityID char_ent = ent_mgr->createEntity();
    auto transform = ent_mgr->addComponentToEntity<Transform>(char_ent);
    auto caps_coll = ent_mgr->addComponentToEntity<CapsuleCollider>(char_ent);
    auto char_cont = ent_mgr->addComponentToEntity<CharacterController>(char_ent);

    caps_coll->is_static = false;
    caps_coll->setOnBodyEnterCallback(
            [char_cont] (CollisionEvent event) {
                char_cont->onCollisionEnter(event);
            });
    caps_coll->setOnBodyLeaveCallback(
            [char_cont] (EntityID other) {
                char_cont->onCollisionLeave(other);
            });

    transform->SetPosition(spawn_position);

    char_cont->gravity_multiplier = 50.0;

    return char_ent;
}

Result<VoidResult, GameError> CharacterController::Init(Game *game, EntityManager *entityManager, EntityID self) {
    this->game = game;
    this->transform = entityManager->getEntityComponent<Transform>(self);
    this->caps_coll = entityManager->getEntityComponent<CapsuleCollider>(self);

    auto ents = entityManager->tag.getEntitiesWithTag("input-handler");
    if ( ents.size() == 0 )
        return ResultError("There's no entity with tag 'input-handler'");
    this->input_handler = entityManager->getEntityComponent<InputHandler>(ents[0]);
    return ResultOK;
}


Result<VoidResult, GameError> CharacterController::Update(double deltaTime_s, EntityManager *entityManager, EntityID self) {
    m_move_input.x = input_handler->getAxisInput("move_x");
    m_move_input.y = input_handler->getAxisInput("move_y");

    return ResultOK;
}


Result<VoidResult, GameError> CharacterController::FixedUpdate(double deltaTime_s, EntityManager *entityManager, EntityID self) {

    {
    const Vector2 position = transform->GlobalPosition();
    const bool is_colliding = game->physics.isEntityColliding(self);
    auto coll_events = game->physics.getEntityCollisionEvents(self);

    bool is_on_floor = false;
    if (is_colliding) {
        for (auto& event: coll_events) {
            Vector2 normal = -1.0 * event.manifold.normal;
            if (normal.y < 0.0 ) {
                if (this->velocity.y > .0) {
                    this->velocity.y = 0;
                }
                is_on_floor = true;
            }
            if (normal.y > 0.0 && std::abs(normal.x) < 0.01) {
                this->velocity.y = - 0.4 * this->velocity.y;
            }
            transform->SetPosition(position + std::max(event.manifold.penetration_depth - 0.5, 0.0) * normal);
        }
    }

    if ( is_on_floor && input_handler->getButtonInput("jump") ) {
        this->velocity.y = Vector2::UP.y * jump_force;
        transform->SetPosition(transform->GlobalPosition() + Vector2::DOWN * velocity.y * deltaTime_s);
    }
        
    if ( !is_on_floor ) {
        this->velocity.y += gravity_multiplier * game->physics.GRAVITY * deltaTime_s;
        transform->SetPosition(transform->GlobalPosition() + Vector2::DOWN * velocity.y * deltaTime_s);
    }

    }

    {
    const Vector2 position = transform->GlobalPosition();
    if ( m_move_input.sqrMagnitude() > .001 ) {
        Vector2 move_dir = Vector2(m_move_input.x, -m_move_input.y).normalized();

        float move_speed = run_speed;
        if (m_move_input.magnitude() < .78) {
            move_speed = walk_speed;
        }
        
        const Vector2 new_position = position + Vector2(move_dir.x, .0) * move_speed * deltaTime_s;
        transform->SetPosition(new_position);
    }
    }

    return ResultOK;
}

void CharacterController::onCollisionEnter(CollisionEvent event) {
}

void CharacterController::onCollisionLeave(EntityID other) {
}


