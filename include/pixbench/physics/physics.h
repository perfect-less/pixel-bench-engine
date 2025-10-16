#ifndef PHYSICS_HEADER
#define PHYSICS_HEADER

#include "pixbench/entity.h"
#include "pixbench/physics/type.h"
#include "pixbench/components.h"
#include "pixbench/vector2.h"
#include <cstddef>
#include <cstdlib>
#include <vector>


// ========== FORWARD DECLARATION ========== BEGIN
class Game;
// ========== FORWARD DECLARATION ========== END


/**
 * Physics API means to be used by gameplay codes.
 */
class PhysicsAPI {
private:
    Game* m_game{ nullptr };
public:
    float GRAVITY = 30.0;

    /**
     * cast a ray, returns the first contact point between the ray and collider in the game.
     */
    bool rayCast(Vector2 origin, Vector2 direction, float length, RaycastHit* out__raycast_hit=nullptr, const EntityID* ignore_entity=nullptr);

    /**
     * cast a circle ray, returns the first contact point between the ray and collider in the game.
     */
    bool circleCast(Vector2 origin, Vector2 direction, float length, float radius, RaycastHit* out__raycast_hit=nullptr, const EntityID* ignore_entity=nullptr);

    /**
     * Perform collision check agains all collider in the game
     */
    std::vector<CollisionEvent> performCollisionCheck(EntityID ent, Collider* collider);

    /**
     * Get number of entity that have Collider component
     */
    size_t numberOfEntityWithCollider();

    /**
     * Returns whether the entity is colliding or ont
     */
    bool isEntityColliding(EntityID ent_id);

    /**
     * Returns vector of CollisionEvent attributed to an entity
     */
    std::vector<CollisionEvent> getEntityCollisionEvents(EntityID ent_id);


    void __setGame(Game* game) {
        m_game = game;
    }
};


bool boxToBoxCollision(BoxCollider* box_1, BoxCollider* box_2, CollisionManifold* manifold__out, bool* is_body_1_the_ref);

bool boxToCircleCollision(BoxCollider* box, CircleCollider* circle, CollisionManifold* manifold__out, bool* is_body_1_the_ref);

bool boxToPolygonCollision(BoxCollider* box, PolygonCollider* polygon, CollisionManifold* manifold__out, bool* is_body_1_the_ref);

bool boxToCapsuleCollision(BoxCollider* box, CapsuleCollider* capsule, CollisionManifold* manifold__out, bool* is_body_1_the_ref);


bool circleToCircleCollision(CircleCollider* circle_1, CircleCollider* circle_2, CollisionManifold* manifold__out, bool* is_body_1_the_ref);

bool circleToPolygonCollision(CircleCollider* circle, PolygonCollider* polygon, CollisionManifold* manifold__out, bool* is_body_1_the_ref);

bool circleToCapsuleCollision(CircleCollider* circle, CapsuleCollider* capsule, CollisionManifold* manifold__out, bool* is_body_1_the_ref);


bool polygonToPolygonCollision(PolygonCollider* polygon_1, PolygonCollider* polygon_2, CollisionManifold* manifold__out, bool* is_body_1_the_ref);

bool polygonToCapsuleCollision(PolygonCollider* polygon, CapsuleCollider* capsule, CollisionManifold* manifold__out, bool* is_body_1_the_ref);


bool capsuleToCapsuleCollision(CapsuleCollider* capsule_1, CapsuleCollider* capsule_2, CollisionManifold* manifold__out, bool* is_body_1_the_ref);


// === Physics Checks ===

/**
 * A handler that points to a collider along with its other attributes.
 */
struct ColliderObject {
    EntityID entity;
    Collider* collider = nullptr;
    ColliderTag collider_tag;
};

bool colliderCheckCollision(Collider* coll, std::vector<ColliderObject> all_colliders);


// === Physics utility functions ===

bool axisAlignedBoundingSquareCheck(
        const Vector2* b1_pos, float b1_radius,
        const Vector2* b2_pos, float b2_radius
        );

Vector2 projectPointToLine(Vector2 point, Vector2 line_p1, Vector2 line_p2);


// === Physics Debug Drawing ===

/**
 * Draw cross using SDL_RenderLine.
 * Use SDL_SetRenderDrawColorFloat before calling this function to set color.
 */
void phydebDrawCross(
        SDL_Renderer* renderer,
        Vector2* center,
        float cross_width = 12.0
        );

#endif
