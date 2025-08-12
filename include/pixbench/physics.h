#ifndef PHYSICS_HEADER
#define PHYSICS_HEADER

#include "pixbench/entity.h"
#include "pixbench/engine_config.h"
#include "pixbench/vector2.h"
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <unordered_map>


class Game;


class RaycastHit {
public:
    Vector2 point;
    Vector2 normal;
    EntityID ent;

    RaycastHit() = default;

    RaycastHit(Vector2 point, Vector2 normal, EntityID ent)
        : point(point), normal(normal.normalized()), ent(ent) {}
};


class PhysicsAPI {
private:
    Game* m_game{ nullptr };
public:
    float GRAVITY = 30.0;

    /*
     * cast a ray, returns the first contact point between the ray and collider in the game.
     */
    bool rayCast(Vector2 origin, Vector2 direction, float length, RaycastHit* out__raycast_hit);

    /*
     * cast a circle ray, returns the first contact point between the ray and collider in the game.
     */
    bool circleCast(Vector2 origin, Vector2 direction, float length, float radius, RaycastHit* out__raycast_hit);


    void __setGame(Game* game) {
        m_game = game;
    }
};


class CollisionManifold {
public:
    Vector2 normal = Vector2::RIGHT;
    float penetration_depth = 0.0;
    size_t point_count = 0;
    Vector2 points[2];

    CollisionManifold() = default;

    CollisionManifold(
            Vector2 normal,
            float penetration_depth
            )
        : 
        normal(normal),
        penetration_depth(penetration_depth)
    {};

    CollisionManifold(const CollisionManifold& source) {
        this->normal = source.normal;
        this->penetration_depth = source.penetration_depth;
        this->setPoints(source.points, source.point_count);
    }

    void setPoints(const Vector2 collision_points[], size_t counts) {
        if ( counts > 2 || counts <= 0 )
            return;

        this->point_count = counts;
        for (int i=0; i<counts; ++i) {
            this->points[i] = collision_points[i];
        }
    }

    void flipNormal() {
        normal = normal * -1.0;
    }
};


class CollisionEvent {
public:
    EntityID other;
    CollisionManifold manifold;

    CollisionEvent(EntityID other, CollisionManifold manifold)
        : other(other), manifold(manifold)
    {};
};



class CollisionManifoldStore {
public:
    CollisionManifold manifold;
    EntityID reference_entity;
};



class CollisionManifoldStorage {
private:
    std::unordered_map<size_t, CollisionManifoldStore> m_manifold_map;
public:

    CollisionManifoldStorage() {
        m_manifold_map.reserve(MAX_ENTITIES*2);
    }

    size_t pairToHashIndex(size_t ent_a, size_t ent_b) {
        if ( ent_b < ent_a ) {
            const size_t temp_a = ent_a;
            ent_a = ent_b;
            ent_b = temp_a;
        }

        return ent_a*MAX_ENTITIES + ent_b;
    }

    bool isManifoldExist(size_t ent_a, size_t ent_b) {
        const size_t hash_key = pairToHashIndex(ent_a, ent_b);
        auto res_it = m_manifold_map.find(hash_key);
        return ( res_it != m_manifold_map.end() );
    }

    CollisionManifoldStore* getManifold(size_t ent_a, size_t ent_b) {
        const size_t hash_index = pairToHashIndex(ent_a, ent_b);
        auto res_it = m_manifold_map.find(hash_index);
        if ( res_it != m_manifold_map.end() ) {
            return &(res_it->second);
        }

        m_manifold_map[hash_index] = CollisionManifoldStore();
        return &(m_manifold_map[hash_index]);
    }

    void removeManifoldPair(size_t ent_a, size_t ent_b) {
        if ( !isManifoldExist(ent_a, ent_b) )
            return;

        const size_t hash_key = pairToHashIndex(ent_a, ent_b);
        m_manifold_map.erase(hash_key);
    }
};


struct Edge {
    Vector2 p1;
    Vector2 p2;
    Vector2 normal;
};


/*
 * Polygon is a list of points that should resulted in a convex body
 * when connected in a counter-clockwise direction.
 */
class Polygon {
private:
public:
    Vector2 vertex[MAX_POLYGON_VERTEX];
    Vector2 centroid = Vector2::ZERO;
    size_t vertex_counts = 0;

    void setVertex(Vector2 vertex[], size_t counts) {
        if ( counts > MAX_POLYGON_VERTEX )
            return;

        this->vertex_counts = counts;
        for (int i=0; i<counts; ++i) {
            this->vertex[i] = vertex[i];
        }
        calculateCentroid();
    }

    void calculateCentroid() {
        float x = 0;
        float y = 0;
        for (int i=0; i<this->vertex_counts; ++i) {
            x += this->vertex[i].x / vertex_counts;
            y += this->vertex[i].y / vertex_counts;
        }
        centroid.x = x;
        centroid.y = y;
    }

    Vector2 getVertex(size_t index, Vector2 offset=Vector2::ZERO, double rotation=0) {
        return vertex[index].rotated(rotation) + offset;
    }

    Vector2 getCentroid(Vector2 offset=Vector2::ZERO, double rotation=0) {
        return centroid.rotated(rotation) + offset;
    }

    Edge getEdge(size_t index, Vector2 offset=Vector2::ZERO, double rotation=0) {
        Vector2 p1, p2, normal;
        
        p1 = vertex[index];
        p2 = vertex[(index+1) % vertex_counts];

        if ( rotation != 0.0 ) {
            p1.rotate(rotation);
            p2.rotate(rotation);
        }

        normal = (p2 - p1).rotated((90.0/180.0) * M_PI).normalized();

        Edge edge;
        edge.p1 = p1 + offset;
        edge.p2 = p2 + offset;
        edge.normal = normal;

        return edge;
    }

    bool isConvex() {
        if ( vertex_counts == 0 )
            return false;

        for (int i=0; i<vertex_counts; ++i) {
            const Edge ref_edge = getEdge(i);
            const Vector2 normal = ref_edge.normal;
            
            for (int j=0; j<vertex_counts; ++j) {
                if ( j == i || j == (i+1)%vertex_counts )
                    continue;

                const double angle = Vector2::AngleBetween(
                        normal, (vertex[j] - getVertex(i).normalized()));
                if (std::abs(angle) < M_PI/2.0) {
                    return false;
                }
            }
        }


        return true;
    }
};

class BoxCollider;
class CircleCollider;
class PolygonCollider;


bool boxToBoxCollision(BoxCollider* box_1, BoxCollider* box_2, CollisionManifold* manifold__out, bool* is_body_1_the_ref);

bool boxToCircleCollision(BoxCollider* box, CircleCollider* circle, CollisionManifold* manifold__out, bool* is_body_1_the_ref);

bool boxToPolygonCollision(BoxCollider* box, PolygonCollider* polygon, CollisionManifold* manifold__out, bool* is_body_1_the_ref);


bool circleToCircleCollision(CircleCollider* circle_1, CircleCollider* circle_2, CollisionManifold* manifold__out, bool* is_body_1_the_ref);

bool circleToPolygonCollision(CircleCollider* circle, PolygonCollider* polygon, CollisionManifold* manifold__out, bool* is_body_1_the_ref);


bool polygonToPolygonCollision(PolygonCollider* polygon_1, PolygonCollider* polygon_2, CollisionManifold* manifold__out, bool* is_body_1_the_ref);


// === Physics utility functions ===

bool axisAlignedBoundingSquareCheck(
        const Vector2* b1_pos, float b1_radius,
        const Vector2* b2_pos, float b2_radius
        );

Vector2 projectPointToLine(Vector2 point, Vector2 line_p1, Vector2 line_p2);


// === Physics Debug Drawing ===

/*
 * Draw cross using SDL_RenderLine.
 * Use SDL_SetRenderDrawColorFloat before calling this function to set color.
 */
void phydebDrawCross(
        SDL_Renderer* renderer,
        Vector2* center,
        float cross_width = 12.0
        );

#endif
