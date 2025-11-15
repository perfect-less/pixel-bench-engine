#ifndef SYSTEMS_HEADER
#define SYSTEMS_HEADER


#include "pixbench/components.h"
#include "pixbench/ecs.h"
// #include "pixbench/engine_config.h"
#include "engine_config.h"
#include "pixbench/entity.h"
#include "pixbench/game.h"
#include "pixbench/physics/physics.h"
#include "pixbench/physics/type.h"
#include "pixbench/utils/results.h"
#include <cstddef>
#include <functional>
class ISystem {
public:
    virtual Result<VoidResult, GameError> Initialize(Game* game, EntityManager* entity_mgr) { return ResultOK; };
    virtual Result<VoidResult, GameError> Awake(EntityManager* entity_mgr) { return ResultOK; };
    virtual Result<VoidResult, GameError> OnComponentAddedToEntity(const ComponentDataPayload* component_info, EntityID entity_id) { return ResultOK; };
    virtual Result<VoidResult, GameError> OnComponentRegistered(const ComponentDataPayload* component_info) { return ResultOK; };
    virtual Result<VoidResult, GameError> OnEvent(SDL_Event *event, EntityManager* entity_mgr) { return ResultOK; };
    virtual Result<VoidResult, GameError> Update(double delta_time_s, EntityManager* entity_mgr) { return ResultOK; };
    virtual Result<VoidResult, GameError> FixedUpdate(double delta_time_s, EntityManager* entity_mgr) { return ResultOK; };
    virtual Result<VoidResult, GameError> LateUpdate(double delta_time_s, EntityManager* entity_mgr) { return ResultOK; };
    virtual Result<VoidResult, GameError> PreDraw(RenderContext* renderContext, EntityManager* entity_mgr) { return ResultOK; };
    virtual Result<VoidResult, GameError> Draw(RenderContext* renderContext, EntityManager* entity_mgr) { return ResultOK; };
    virtual Result<VoidResult, GameError> OnEntityDestroyed(EntityManager* entity_mgr, EntityID entity_id) { return ResultOK; };
    virtual Result<VoidResult, GameError> OnError(EntityManager* entity_mgr) { return ResultOK; };
    virtual Result<VoidResult, GameError> OnExit(EntityManager* entity_mgr) { return ResultOK; };
};


class HierarchySystem : public ISystem {
private:
    std::unordered_map<EntityIDNumber, std::vector<EntityID>> m_child_store;
    Game* m_game = nullptr;
    Transform m_empty_transform;
public:
    HierarchySystem();

    void __setGame(Game* game) { m_game = game; };

    void _addChildToEntity(EntityID parent, EntityID child);

    void _addChildsToEntity(EntityID parent, std::vector<EntityID> childs);

    void _removeChildFromEntity(EntityID parent, EntityID child);

    bool _getEntityChildAtIndex(EntityID parent, size_t index, EntityID* out__child_id);

    void _getEntityChilds(EntityID parent, std::vector<EntityID>& out__childs, bool recursive=false);

    size_t _getEntityChildCount(EntityID parent);

    void _entitySyncing(EntityID parent, Hierarchy* parent_hierarchy, Transform* last_parent_transform);
    void _syncEntityTransform(EntityID parent);

    void _syncAllTransform();

    Result<VoidResult, GameError> Initialize(Game* game, EntityManager* entity_mgr);
    // Result<VoidResult, GameError> OnComponentRegistered(const ComponentDataPayload* component_info);
    // Result<VoidResult, GameError> OnEvent(SDL_Event *event, EntityManager* entity_mgr);
    // Result<VoidResult, GameError> Update(double delta_time_s, EntityManager* entity_mgr);
    // Result<VoidResult, GameError> LateUpdate(double delta_time_s, EntityManager* entity_mgr);
    Result<VoidResult, GameError> FixedUpdate(double delta_time_s, EntityManager* entity_mgr);
    // Result<VoidResult, GameError> OnEntityDestroyed(EntityManager* entity_mgr, EntityID entity_id);
};


class RenderingSystem : public ISystem {
private:
    std::vector<RenderableComponent*> ordered_renderables;
    std::bitset<MAX_COMPONENTS> m_renderable_components_mask;
public:
    Result<VoidResult, GameError> Initialize(Game* game, EntityManager* entity_mgr) override;
    Result<VoidResult, GameError> OnComponentRegistered(const ComponentDataPayload* component_info) override;
    Result<VoidResult, GameError> LateUpdate(double delta_time_s, EntityManager* entity_mgr) override; // Animation update
    Result<VoidResult, GameError> PreDraw(RenderContext* renderContext, EntityManager* entity_mgr) override;
    Result<VoidResult, GameError> Draw(RenderContext* renderContext, EntityManager* entity_mgr) override;
};


class AudioSystem : public ISystem {
private:
    std::vector<AudioChannelState> m_channel_states;
public:
    Result<VoidResult, GameError> Initialize(Game* game, EntityManager* entity_mgr) override;
    Result<VoidResult, GameError> LateUpdate(double delta_time_s, EntityManager* entity_mgr) override; // Audio update
};


class PhysicsSystem : public ISystem {
private:
    std::bitset<MAX_COMPONENTS> m_physics_components_mask;
    size_t m_collisions_count[MAX_ENTITIES];
    EntityID m_entities_with_collider[MAX_ENTITIES];
    size_t m_num_entities_with_collider{ 0 };
    CollisionManifoldStorage m_manifolds;
    ColliderObject m_collider_objects[MAX_ENTITIES];
    size_t m_collider_objects_count = 0;
public:
    PhysicsSystem();

    bool isPairColliding(EntityID ent_a, EntityID ent_b);
    bool isEntityColliding(EntityID ent_id);
    CollisionManifoldStore* getCollisionPair(EntityID ent_a, EntityID ent_b);
    std::vector<CollisionEvent> getEntityCollisionManifolds(EntityID ent_id);
    void setCollisionPair(EntityID ent_a, EntityID ent_b, CollisionManifold* manifold, EntityID ref_entity);
    void removeCollisionPair(EntityID ent_a, EntityID ent_b);

    size_t __numEntitiesWithCollider() { return m_num_entities_with_collider; };
    void __updateColliderObjectList(EntityManager* entity_mgr);
    void __colliderCheckCollision(
            ColliderObject* collider,
            std::function<void (
                bool is_colliding, CollisionManifold* manifold, bool* is_body_1_ref, ColliderObject* coll_1, ColliderObject* coll_2)
            > result_handling_function = nullptr,
            size_t __collider_object_start_index = 0
            );

    std::bitset<MAX_COMPONENTS> __getPhysicsComponentMask() { return m_physics_components_mask; }

    Result<VoidResult, GameError> Initialize(Game* game, EntityManager* entity_mgr) override;
    Result<VoidResult, GameError> FixedUpdate(double delta_time_s, EntityManager* entity_mgr) override; // Physics update
    Result<VoidResult, GameError> OnComponentAddedToEntity(const ComponentDataPayload* component_info, EntityID entity_id) override;
    Result<VoidResult, GameError> OnComponentRegistered(const ComponentDataPayload* component_info) override;
    Result<VoidResult, GameError> Draw(RenderContext* renderContext, EntityManager* entity_mgr) override;
    Result<VoidResult, GameError> OnEntityDestroyed(EntityManager* entity_mgr, EntityID entity_id) override;
};


class ScriptSystem : public ISystem {
private:
    std::bitset<MAX_COMPONENTS> m_script_components_mask;

public:
    ScriptSystem();

    Result<VoidResult, GameError> Initialize(Game* game, EntityManager* entity_mgr);
    Result<VoidResult, GameError> OnComponentRegistered(const ComponentDataPayload* component_info);
    Result<VoidResult, GameError> OnEvent(SDL_Event *event, EntityManager* entity_mgr);
    Result<VoidResult, GameError> Update(double delta_time_s, EntityManager* entity_mgr);
    Result<VoidResult, GameError> LateUpdate(double delta_time_s, EntityManager* entity_mgr);
    Result<VoidResult, GameError> FixedUpdate(double delta_time_s, EntityManager* entity_mgr);
    Result<VoidResult, GameError> OnEntityDestroyed(EntityManager* entity_mgr, EntityID entity_id);
};


#endif
