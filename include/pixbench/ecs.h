#ifndef ECS_HEADER
#define ECS_HEADER

#include "SDL3_mixer/SDL_mixer.h"
#include "pixbench/game.h"
#include "pixbench/physics.h"
#include "pixbench/resource.h"
#include "pixbench/vector2.h"
#include "pixbench/renderer.h"
#include "pixbench/engine_config.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_surface.h>
#include <array>
#include <bitset>
#include <cstddef>
#include <cwchar>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <strings.h>
#include <sys/types.h>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

class EntityManager;
class IComponent;
class ISystem;

typedef size_t EntityIDNumber;

struct EntityID {
    EntityIDNumber id;
    size_t version = 0;
};


enum ComponentTag {
    CTAG_Generic,
    CTAG_Renderable,
    CTAG_Script,
    CTAG_AudioPlayer,
    CTAG_Collider
};


class IComponent {
public:
    // Component tag
    virtual ComponentTag getCTag() const {return CTAG_Generic;};
    static ComponentTag getComponentTag() {return CTAG_Generic;};
};

class GameObject : public IComponent{
private:
    std::string name;
public:

    GameObject(std::string name) {
        this->name = name;
    };

    GameObject() : name("unnamed") {};

    std::string Name() {return this->name;};
    void SetName(std::string name) {this->name = name;};
};

class Transform : public IComponent {
private:
    Vector2 localPosition = Vector2();
    Vector2 globalPosition = Vector2();

public:
    double rotation;

    Transform() = default;
    Transform(const Transform& source) {
        this->SetPosition(source.globalPosition);
        this->rotation = source.rotation;
    }

    void SetPosition(Vector2 position);
    void SetLocalPosition(Vector2 position);

    Vector2 LocalPosition() {return this->localPosition;};
    Vector2 GlobalPosition() {return this->globalPosition;};
};


enum ColliderTag {
    COLTAG_Box,
    COLTAG_Circle,
    COLTAG_Polygon
};

class Collision {
public:
    EntityID body_a;
    EntityID body_b;
    bool is_body_a_the_ref = false;
    CollisionManifold manifold;

    Collision(EntityID body_a, EntityID body_b, CollisionManifold manifold)
        : body_a(body_a), body_b(body_b), manifold(manifold)
    {};
};

class CollisionEvent {
public:
    EntityID other;
    CollisionManifold manifold;
    
    CollisionEvent(EntityID other, CollisionManifold manifold)
        : other(other), manifold(manifold)
    {};
};

class Collider : public IComponent {
private:
    std::function<void(CollisionManifold&)> m_on_body_enter_callback = nullptr;
    std::function<void(CollisionManifold&)> m_on_body_leave_callback = nullptr;
    CollisionManifold m_manifold;
public:
    float skin_depth = 1.0;             //!< skin depth for collision detection
    Transform __transform = Transform();

    void setManifold(CollisionManifold& manifold) {
        m_manifold.penetration_depth = manifold.penetration_depth;
        m_manifold.point_count = manifold.point_count;
        m_manifold.normal = manifold.normal;
        m_manifold.setPoints(manifold.points, manifold.point_count);
    }

    CollisionManifold& getManifold() {
        return m_manifold;
    }

    virtual ColliderTag getColliderTag() const {
        return COLTAG_Box;
    };

    virtual void setOnBodyEnterCallback(
            std::function<void(CollisionManifold&)> callback_function
            ) {
        m_on_body_enter_callback = callback_function;
    }

    virtual void setOnBodyLeaveCallback(
            std::function<void(CollisionManifold&)> callback_function
            ) {
        m_on_body_leave_callback = callback_function;
    }

    void __triggerOnBodyEnter() {
        if ( m_on_body_enter_callback )
            m_on_body_enter_callback(m_manifold);
    }

    void __triggerOnBodyLeave() {
        if ( m_on_body_leave_callback )
            m_on_body_leave_callback(m_manifold);
    }

    ComponentTag getCTag() const override {
        return CTAG_Collider;
    };
    static ComponentTag getComponentTag() {
        return CTAG_Collider;
    };
};


class BoxCollider : public Collider {
public:
    float width =  16.0;            //!< box collider width
    float height = 16.0;            //!< box collider height
    
    Polygon __polygon;

    BoxCollider() {
        setSize(this->width, this->height);
    }

    void setSize() {
        Vector2 verts[4];
        verts[0] = Vector2(this->width / 2.0, -this->height / 2.0);
        verts[1] = Vector2(-this->width / 2.0, -this->height / 2.0);
        verts[2] = Vector2(-this->width / 2.0, this->height / 2.0);
        verts[3] = Vector2(this->width / 2.0, this->height / 2.0);
        this->__polygon.setVertex(verts, 4);
    }

    void setSize(float width, float height) {
        this->width = width;
        this->height = height;

        this->setSize();
    }

    ColliderTag getColliderTag() const override {
        return COLTAG_Box;
    };
};


class CircleCollider : public Collider {
public:
    float radius = 16.0;            //!< circle collider radius
    
    ColliderTag getColliderTag() const override {
        return COLTAG_Box;
    };
};


class PolygonCollider : public Collider {
public:
    Polygon __polygon;

    /*
     * Set polygon
     * returns: true when successfull (polygon is convex), false otherwise
     */
    bool setPolygon(Polygon polygon) {
        if ( !polygon.isConvex() )
            return false;

        this->__polygon.setVertex(
                polygon.vertex,
                polygon.vertex_counts
                );
        return true;
    }
    
    ColliderTag getColliderTag() const override {
        return COLTAG_Polygon;
    };
};


enum RenderableTag {
    RCTAG_Sprite,
    RCTAG_Tile,
    RCTAG_Script
};

class RenderableComponent : public IComponent {
private:
public:
    float depth = 1;                                       // depth for render order
    SDL_FRect srect;                                       // source rect for SDL_Render, animation will just change this 
    SDL_FRect drect;                                       // destination rect for SDL_Render,
                                                           // :Sprite use this as render target on the screen
                                                           // :Tile, CustomRenderable will only use this for visibility check

    Transform* transform = nullptr;        // renderables requires transform for positioning

    virtual RenderableTag getRenderableTag() const {
        return RCTAG_Sprite;
    };

    ComponentTag getCTag() const override {
        return CTAG_Renderable;
    };
    static ComponentTag getComponentTag() {
        return CTAG_Renderable;
    };

};

class Sprite : public RenderableComponent {
private:
    int m_max_frame = 0;
    std::shared_ptr<SpriteAnimation> sprite_anim = nullptr;
public:
    int current_frame = 0;
    double current_anim_time_s = 0;

    double animation_speed_multiplier = 1;                  //!< animation speed multiplier (multiplied on top of SpriteAnimation' speed)
    SDL_FlipMode flip_mode = SDL_FlipMode::SDL_FLIP_NONE;   //!< flip mode for the renderables
    Vector2 offset;                                         //!< offset for rendering Sprite relative to transform
    std::shared_ptr<Res_SDL_Texture> texture = nullptr;     //!< texture to render

    bool paused = false;
    bool is_animation_finished = false;

    RenderableTag getRenderableTag() const override {
        return RCTAG_Sprite;
    };

    void SetSpriteAnim(
            std::shared_ptr<SpriteAnimation> sprite_anim,
            bool reset_anim = true
            ) {
        this->sprite_anim = sprite_anim;
        this->texture = sprite_anim->res_sheet->res_texture;
        this->m_max_frame = 1 + sprite_anim->end_sheet_index - sprite_anim->start_sheet_index;
        if (reset_anim)
            this->ResetAnimation();
    };

    int maxFrame() {
        return m_max_frame;
    }

    void ResetAnimation() {
        this->current_frame = 0;
        this->current_anim_time_s = 0;
    }

    bool isAnimationFinished() {
        return is_animation_finished;
    }

    void SetTexture(
            std::shared_ptr<Res_SDL_Texture> texture,
            bool set_rect_to_cover_whole_texture = true
            ) {
        this->texture = texture;
        if (set_rect_to_cover_whole_texture) {
            this->SetSrcAndDstRectToCoverWholeTexture();
        }
    };

    void SetSrcAndDstRectToCoverWholeTexture() {
        if (!this->texture) 
            return;

        this->srect.x = 0;
        this->srect.y = 0;
        this->srect.w = this->texture->texture->w;
        this->srect.h = this->texture->texture->h;

        this->drect.w = this->texture->texture->w;
        this->drect.h = this->texture->texture->h;
    }

    int texture_width() {
        if (this->texture)
            return texture->texture->w;
        return 0;
    }

    int texture_height() {
        if (this->texture)
            return texture->texture->h;
        return 0;
    }

    std::weak_ptr<SpriteAnimation> GetSpriteAnimation() {
        return this->sprite_anim;
    };
};


class Tile : public RenderableComponent {
private:
    std::shared_ptr<TileMapLayer> m_tile_map_layer = nullptr;
public:
    RenderableTag getRenderableTag() const override {
        return RCTAG_Tile;
    };
    
    void setTileMapLayer(std::shared_ptr<TileMapLayer> tile_map) {
        m_tile_map_layer = tile_map;
    }
    
    std::weak_ptr<TileMapLayer> getTileMap() {
        return m_tile_map_layer;
    }
};


class CustomRenderable : public RenderableComponent {
private:
public:
    Transform* transform = nullptr;
    SDL_FRect offset = {
        0.0f,   // x
        0.0f,   // y
        32.0f,  // width
        32.0f   // height
    };

    RenderableTag getRenderableTag() const override {
        return RCTAG_Script;
    };

    virtual void Init(Game* game, EntityManager* entityManager, EntityID self) { };
    virtual void Draw(RenderContext* renderContext, EntityManager* entity_mgr) { };
};


class AudioPlayer : public IComponent {
private:
    int m_assigned_channel{ -1 };
    double m_position{ 0.0 };
    double m_last_known_position{ 0.0 };
    bool m_is_playing{ false };
    bool m_is_need_to_replay{ false };
    bool m_is_paused{ false };
    bool m_is_finished{ false };

    bool m_is_need_sync{ false };
public:
    ComponentTag getCTag() const override {
        return CTAG_AudioPlayer;
    };

    static ComponentTag getComponentTag() {
        return CTAG_AudioPlayer;
    };

    std::shared_ptr<AudioClip> clip{ nullptr };
    int volume{ MIX_MAX_VOLUME };   //!< Volume for audio played by this player
    bool is_looping{ false };       //!< Set `true` to loop the audio
                                    //
    /*
     * Get assigned playback channel of this player. It has none if this returns -1
     */
    inline int getAssignedChannel() {
        return m_assigned_channel;
    }

    /*
     * Internal methods
     */
    int __setAssignedChannel(int channel);
    bool __isNeedSync();
    void __resetNeedSyncStatus();
    void __setIsPlaying(bool is_playing);
    void __setIsFinished(bool is_finished);

    inline bool __isNeedToReplay() {
        return m_is_need_to_replay;
    }

    /*
     * Set the audio clip
     */
    void setClip(std::shared_ptr<AudioClip> audio_clip);

    /*
     * Get audio position in seconds
     */
    double getPosition();

    /*
     * Returns `true` if this player is currently playing audio
     */
    bool isPlaying();

    /*
     * Returns `true` if this player has finished playing an audio clip
     */
    bool isFinished();

    /*
     * Returns `true` if this player is currently paused
     */
    bool isPaused();

    /*
     * Play the audio clip, specify at (from 0 to 1) as position to start the audio
     * play.
     */
    void play();

    /*
     * Pause the currently playing audio.
     */
    void pause();

    /*
     * Resume paused audio at the last known position.
     */
    void resume();

    /*
     * Stop audio play
     */
    void stop();

    /*
     * Pause if audio is playing, otherwise play the audio clip.
     */
    void togglePlay();
};


class ScriptComponent : public IComponent {
public:

    ComponentTag getCTag() const override {
        return CTAG_Script;
    };

    static ComponentTag getComponentTag() {
        return CTAG_Script;
    };

    virtual Void Init(Game* game, EntityManager* entityManager, EntityID self) { return ResultOK; };
    virtual Void Update(double deltaTime_s, EntityManager* entityManager, EntityID self) { return ResultOK; };
    virtual Void LateUpdate(double deltaTime_s, EntityManager* entityManager, EntityID self) { return ResultOK; };
    virtual Void FixedUpdate(double deltaTime_s, EntityManager* entityManager, EntityID self) { return ResultOK; };
    virtual Void Draw(RenderContext* renderContext, EntityManager* entity_mgr, EntityID self) { return ResultOK; };
    virtual Void OnEvent(SDL_Event* event, EntityManager* entityManager, EntityID self) { return ResultOK; };
    virtual Void OnDestroy(EntityManager* entityManager, EntityID self) { return ResultOK; };
};


struct EntityInfo {
    EntityID entityid;                              //!< Unique entity's identifier
    bool active = false;                            //!< active status, true means this entity is active
    size_t current_version = 0;                     //!< version, to prevent old disabled id to be used by user
    std::bitset<MAX_COMPONENTS> component_mask;     //!< mask of assigned components to this entity
};


/**
 * Required Interface to make sure that ComponentManager can keep reference to
 * multiple types of ComponentArray
 */
class IComponentArray {
public:
    virtual ~IComponentArray() = default;
    virtual void handleEntityDestroyed(EntityIDNumber entity_id) = 0;
    virtual IComponent* getIComponentById(EntityIDNumber entity_id) = 0;
};


/**
 * Container for components of types T that were managed by ComponentManager.
 *
 * The components were stored in a `vector<T>` that will only grow when a T component
 * were added to an entity. This were done to saves on memory usage and avoid sparse
 * array that will contains mostly unused component objects.
 */
template <typename T>
class ComponentArray : public IComponentArray {
private:
    /** map of entity ID to index in m_components */
    std::unordered_map<EntityIDNumber, size_t> m_entity_to_index_map;
    /** map of index in m_components to entity ID */
    std::unordered_map<size_t, EntityIDNumber> m_index_to_entity_map;
    /** array of component objects */
    std::vector<T> m_components;
    /** queue of empty indexes in the m_components */
    std::queue<size_t> m_empty_index_queue;
    /** empty index at the end of the m_components array */
    size_t m_last_empty_component_index = 0;

public:
    /**
     * returns pointer of component T in the arrays of stored components.
     * If the entity doesn't have component T, it will return null pointer.
     * 
     * **Note**: *this pointer can be invalid when component arrays grows.*
     */
    T* getComponentByEntityID(EntityIDNumber entity_id) {
        auto index_pair = m_entity_to_index_map.find(entity_id);
        if (index_pair != m_entity_to_index_map.end()) {
            size_t component_index = index_pair->second;
            return &m_components[component_index];
        }

        return nullptr;
    }

    IComponent* getIComponentById(EntityIDNumber entity_id) {
        return this->getComponentByEntityID(entity_id);
    }

    /**
     * This function won't check whether entity_id is already populated or not,
     * instead it will replaced existing component if that component is already
     * associated with an entity.
     * it is the responsibility of the caller to check that.
     */
    void addComponentToArray(EntityIDNumber entity_id) {
        size_t new_index;
        if (this->m_empty_index_queue.size() > 0) {
            new_index = this->m_empty_index_queue.front();
            this->m_empty_index_queue.pop();

            // replace object on the old index with new ones
            m_components[new_index].~T();
            new (&m_components[new_index]) T();

        } else {
            new_index = m_last_empty_component_index;
            m_last_empty_component_index++;

            m_components.push_back(T());
            std::cout << "typename: " << typeid(T).name() << std::endl;
            std::cout << "New Component pushed back, comps.size()=" << m_components.size() << std::endl;
        }

        m_entity_to_index_map[entity_id] = new_index;
        m_index_to_entity_map[new_index] = entity_id;
        
        for (auto ent_ind_pair : m_entity_to_index_map) {
            std::cout << "  " << ent_ind_pair.first << " -> " << ent_ind_pair.second << std::endl;
            std::cout << std::endl;
        }
    }

    /**
     * This component won't check whether entity_id have any associated component
     * or not and removing non existent component will cause error
     */
    void removeComponentFromArray(EntityIDNumber entity_id) {
        
        size_t component_index = m_entity_to_index_map[entity_id];
        m_entity_to_index_map.erase(entity_id);
        m_index_to_entity_map.erase(component_index);

        m_empty_index_queue.push(component_index);
    }

    /**
     * Remove components from entity if the entity have the component
     */
    void handleEntityDestroyed(EntityIDNumber entity_id) override {
        auto index_pair = m_entity_to_index_map.find(entity_id);
        if (index_pair != m_entity_to_index_map.end()) {
            this->removeComponentFromArray(entity_id);
        }
    }
};


typedef size_t ComponentType;


class ComponentManager {
private:
    std::map<ComponentType, size_t> m_component_type_to_index_map;
    std::map<size_t, ComponentType> m_index_to_component_type_map;

    std::vector<std::shared_ptr<IComponentArray>> m_component_arrays;

public:
    std::function<void(ComponentTag, ComponentType, size_t)> component_registered_callback{ nullptr };

    template<typename T>
    void registerComponent() {
        ComponentType component_type = typeid(T).hash_code();
        
        auto type_index_pair = m_component_type_to_index_map.find(component_type);
        if (type_index_pair == m_component_type_to_index_map.end()) {
            size_t new_index = m_component_arrays.size();
            m_component_type_to_index_map[component_type] = new_index;
            m_index_to_component_type_map[new_index] = component_type;

            auto new_component_array = std::make_shared<ComponentArray<T>>();
            m_component_arrays.push_back(new_component_array);

            if (component_registered_callback)
            {
                T dummy_obj = T();
                ComponentTag ctag = static_cast<IComponent*>(&dummy_obj)->getCTag();
                component_registered_callback(ctag, component_type, new_index);
            }
        }
    }

    template<typename T>
    size_t getComponentIndex() {
        ComponentType component_type = typeid(T).hash_code();
        
        auto type_index_pair = m_component_type_to_index_map.find(component_type);
        if (type_index_pair != m_component_type_to_index_map.end()) {
            return type_index_pair->second;
        } else {
            this->registerComponent<T>();
            return m_component_type_to_index_map[component_type];
        }
    }

    template<typename T>
    std::shared_ptr<ComponentArray<T>> getComponentArray() {
        ComponentType component_type = typeid(T).hash_code();

        auto type_index_pair = m_component_type_to_index_map.find(component_type);
        if (type_index_pair != m_component_type_to_index_map.end()) {
            return std::static_pointer_cast<ComponentArray<T>>(m_component_arrays[type_index_pair->second]);
            // return m_component_arrays[type_index_pair->second];
        }

        return nullptr;
    }
    
    template<typename T>
    void addComponentToEntity(EntityIDNumber entity_id){
        ComponentType component_type = typeid(T).hash_code();

        std::shared_ptr<ComponentArray<T>> component_array = this->getComponentArray<T>();
        if (component_array != nullptr) {
            component_array->addComponentToArray(entity_id);
        }
    }

    template<typename T>
    void removeComponentFromEntity(EntityIDNumber entity_id) {
        std::shared_ptr<ComponentArray<T>> component_array = this->getComponentArray<T>();
        if (component_array != nullptr) {
            component_array->removeComponentFromArray(entity_id);
        }
    };

    template<typename T>
    T* getEntityComponent(EntityIDNumber entity_id){
        ComponentType component_type = typeid(T).hash_code();

        std::shared_ptr<ComponentArray<T>> component_array = this->getComponentArray<T>();
        if (component_array != nullptr) {
            return component_array->getComponentByEntityID(entity_id);
        }

        return nullptr;
    };

    template<typename T>
    T* getEntityComponentCasted(EntityIDNumber entity_id, size_t component_index){
        ComponentType component_type = typeid(T).hash_code();

        auto index_type_pair = m_index_to_component_type_map.find(component_index);
        if (index_type_pair != m_index_to_component_type_map.end()) {
            auto component_array = m_component_arrays[component_index];
            IComponent* icomponent = component_array->getIComponentById(entity_id);
            T* tcomponent = dynamic_cast<T*>(icomponent);
            if (icomponent != nullptr && tcomponent != nullptr) {
                return tcomponent;
            }
        }

        return nullptr;
    };

    void clearEntityComponents(EntityIDNumber entity_id) {
        for (auto &keyval_pair : m_component_type_to_index_map) {
            size_t component_index = keyval_pair.second;
            
            m_component_arrays[component_index]->handleEntityDestroyed(entity_id);
        }
    };
};


/**
 * Data payload struct, meaning it only used to pass around data
 * this one were used to passed component related data when a new component type
 * is registered to ECS.
 */
struct ComponentDataPayload {
    ComponentTag ctag;
    ComponentType ctype;
    size_t cindex;
};


/**
 * This class is the most important interface to the ECS system.
 * EntityManager stored and keep tracks of all entities, their components, and
 * systems associated to said components. More importantly, it provides interface
 * to gain the states of those entities and interacts with ECS.
 */
class EntityManager{
private:

    EntityInfo m_entities[MAX_ENTITIES];                    //!< This is the ultimate source of truth regarding entities
    std::queue<EntityIDNumber> m_unused_entity_queue;       //!< queue of unused entity IDs, take from this when creating new entity if available
    EntityIDNumber m_last_available_entity_number;          //!< counter 
    std::vector<EntityID> m_uninitialized_entities;         //!< for keeping track of uninitialized entity
    ComponentManager* m_component_manager = nullptr;        //!< ComponentManager

    /**
     * callback to `Game` when destroyEntity() was called
     */
    std::function<void(EntityID entity_id)> m_on_entity_destroyed_callback { nullptr };

public:
    
    EntityManager();
    ~EntityManager();

    /**
     * Create a new entity and returns the ID of the newly created entity.
     */
    EntityID createEntity();

    /**
     * return a vector of the IDs all entities
     */
    std::vector<EntityID> getEntities();

    /**
     * return a vector of the IDs all uninitialized entities (their Init function
     * haven't been called yet).
     */
    std::vector<EntityID> getUninitializedEntities();

    /**
     * Remove entity from uninitialized list
     */
    void setEntityAsInitialized(EntityID entity);

    /**
     * Clear the uninitialized entities list
     */
    void resetEntitiesUninitializedStatus();

    /**
     * Set ComponentManager's `component_registered_callback`.
     * This callback were used by ComponentManager to notify upwards towards the `Game` object
     * that there's new component registered, `Game` object can then propagate the signal
     * downwards towards all the systems in ECS.
     *
     * It was done this way to avoid `EntityManager` referencing `Game` object because
     * the header file resolution for that scenario is not easy.
     */
    void setComponentRegisterCallback(
            std::function<void(ComponentTag, ComponentType, size_t)>
            );

    void setOnEntityDestroyedCallback(
            std::function<void(EntityID entity_id)>
            );

    bool isEntityActive(EntityID entity) {
        return m_entities[entity.id].active;
    }

    bool isEntityActive(EntityIDNumber entity_id) {
        return m_entities[entity_id].active;
    }

    /**
     * if this returns `true`, that means you can use the entity ID. If it
     * return `false` it could mean that the ID have never been issued by 
     * entityManager or that the ID have been reused and the entity that had
     * previously used said ID have been deleted.
     */
    bool isEntityValid(EntityID entity) {
        if (!isEntityActive(entity)) {
            return false;
        }
        if (entity.version != m_entities[entity.id].current_version) {
            return false;
        }
        return true;
    }

    /**
     * return the numbers of maximum allowable entities, you can change this
     * by changing the value of MAX_ENTITIES in `include/pixbench/engine_config.h`
     */
    size_t getMaxEntities() {
        return MAX_ENTITIES;
    }

    /**
     * Numbers of currently active entities.
     */
    size_t getActiveEntityCounts() {
        size_t counts = 0;
        for (EntityIDNumber i=0; i<MAX_ENTITIES; ++i) {
            if (isEntityActive(i))
                ++counts;
        }
        return counts;
    }

    EntityInfo& getEntityInfoByIDNumber(EntityIDNumber entity_id) {
        return m_entities[entity_id];
    }
    
    /**
     * Adding component with type T to entity
     * will return nullptr if:
     *   1. entity is not valid, or
     *   2. entity already have said component
     */
    template<typename T>
    T* addComponentToEntity(EntityID entity)
    {
        size_t component_index = m_component_manager->getComponentIndex<T>();
        if (!isEntityValid(entity)) {
            // Entity isn't valid or it isn't active
            return nullptr;
        }

        if (m_entities[entity.id].component_mask[component_index]) {
            // Component already exists
            return nullptr;
        }

        m_component_manager->addComponentToEntity<T>(entity.id);
        m_entities[entity.id].component_mask[component_index] = true;
        
        return getEntityComponent<T>(entity);
    };

    /**
     * will return nullptr if:
     *   1. entity is not valid, or
     *   2. entity already have said component
     */
    template<typename T>
    bool removeComponentFromEntity(EntityID entity)
    {
        // Return 'true' if sucessful, 'false' if not
        size_t component_index = m_component_manager->getComponentIndex<T>();
        if (!isEntityValid(entity)) {
            // Entity isn't valid or it isn't active
            return false;
        }

        if (!m_entities[entity.id].component_mask[component_index]) {
            // Component did not exists
            return false;
        }

        m_component_manager->removeComponentFromEntity<T>(entity.id);
        m_entities[entity.id].component_mask[component_index] = false;

        return true;
    };

    /**
     * Invalid entity ID will also return `false`
     */
    template<typename T>
    bool isEntityHasComponent(EntityID entity)
    {
        if (!isEntityValid(entity)) {
            return false;
        }

        size_t component_index = m_component_manager->getComponentIndex<T>();
        return m_entities[entity.id].component_mask[component_index];
    };

    /**
     * Invalid entity ID will also return `false`
     */
    bool isEntityHasComponent(EntityID entity, size_t component_index)
    {
        if (!isEntityValid(entity)) {
            return false;
        }

        if (m_entities[entity.id].component_mask[component_index] == 1) {
            return m_entities[entity.id].component_mask[component_index];
        }
        return false;
    };

    /**
     * This will remove all components related to an entity and then set the
     * EntityIDD as inactive. The EntityID might be reused for new entity.
     */
    void destroyEntity(EntityID entity) {
        // notify systems
        if (this->m_on_entity_destroyed_callback)
            m_on_entity_destroyed_callback(entity);

        // remove components
        this->m_component_manager->clearEntityComponents(entity.id);
        // deactivate entity
        this->m_entities[entity.id].active = false;
        this->m_entities[entity.id].current_version += 1;
        // add to unused entity
        this->m_unused_entity_queue.push(entity.id);
        // reset component mask
        this->m_entities[entity.id].component_mask.reset();
    };

    /*
     * Destroy all entities available in EntityManager.
     * 
     * Read more on `destroyEntity`
     */
    void destroyAllEntities() {
        for (EntityIDNumber i=0; i<MAX_ENTITIES; ++i) {
            if (isEntityActive(i)) {
                EntityID ent_id = m_entities[i].entityid;
                this->destroyEntity(ent_id);
            }
        }
    }
    
    /**
     * Return 'nullptr' if component doesn't exist, return pointer to
     * component if the component exists
     */
    template <typename T>
    T* getEntityComponent(EntityID entity) {
        
        if (!isEntityHasComponent<T>(entity)) {
            return nullptr;
        }

        return m_component_manager->getEntityComponent<T>(entity.id);
    };

    /**
     * Same as getEntityComponent(EntityID entity)
     * Although in this one, you query the component using the component type index
     * used by ComponentManager.
     */
    template <typename T>
    T* getEntityComponentCasted(EntityID entity, size_t component_index) {
        if (!isEntityValid(entity)) {
            return nullptr;
        }
        if (!m_entities[entity.id].component_mask[component_index]) {
            return nullptr;
        }

        return m_component_manager->getEntityComponentCasted<T>(entity.id, component_index);
    };

    /**
     * Get component type hash of component T used by ComponentManager
     */
    template<typename T>
    ComponentType getComponentIndex() {
        return m_component_manager->getComponentIndex<T>();
    };
};


/**
 * Usage example:
 * ~~~~~~~~~~~~~~~~{.cpp}
 * for (EntityID ent : EntityView(entity_mgr, mask, true)) {
 *     // do stuff
 * }
 * ~~~~~~~~~~~~~~~~
 */
class EntityView {
private:
    EntityManager* m_entity_mgr;
    std::bitset<MAX_COMPONENTS> m_component_mask;
    const bool m_require_all_to_match{ true };
    
public:

    EntityView(
            EntityManager* entity_mgr, std::bitset<MAX_COMPONENTS> component_mask,
            const bool require_all_to_match = true
            )
        : 
            m_entity_mgr(entity_mgr),
            m_component_mask(component_mask),
            m_require_all_to_match(require_all_to_match)
    {};

    void setComponentMask(std::bitset<MAX_COMPONENTS> mask) {
        this->m_component_mask = mask;
    };

    struct Iterator {
        private:
            EntityManager* m_entity_mgr;
            std::bitset<MAX_COMPONENTS> m_component_mask;
            const bool m_require_all_to_match{ true };
        public:
            EntityIDNumber current_ent_id = 0;

            Iterator(
                    EntityManager* entity_manager,
                    EntityIDNumber start_id, 
                    std::bitset<MAX_COMPONENTS> component_mask,
                    const bool require_all_to_match
                    )
                : 
                    m_entity_mgr(entity_manager), 
                    m_component_mask(component_mask),
                    m_require_all_to_match(require_all_to_match),
                    current_ent_id(start_id)
            {
                if (current_ent_id > m_entity_mgr->getMaxEntities()) {
                    current_ent_id = m_entity_mgr->getMaxEntities();
                } // making sure ~.end() will always be the same
                moveForwardUntilMatched();
            };

            EntityID operator*() {
                return m_entity_mgr->getEntityInfoByIDNumber(current_ent_id).entityid;
            };

            bool operator==(const Iterator& other) {
                return this->current_ent_id == other.current_ent_id;
            };
            bool operator!=(const Iterator& other) {
                return this->current_ent_id != other.current_ent_id;
            };

            const bool isCurrentEntityMatched() {
                // all required bits should be set for the function to return
                // 'true'
                if (m_require_all_to_match) {
                    return (
                            m_entity_mgr->isEntityActive(current_ent_id) 
                            &&
                            (
                             m_entity_mgr->getEntityInfoByIDNumber(current_ent_id).component_mask & m_component_mask
                            ) == m_component_mask
                           );
                }

                // just one bit in the EntityInfo coresponds with mask will
                // return 'true'
                return (
                        m_entity_mgr->isEntityActive(current_ent_id) 
                        &&
                        (
                         m_entity_mgr->getEntityInfoByIDNumber(current_ent_id).component_mask & m_component_mask
                        ).count() >= 1
                       );
            }

            void moveForwardUntilMatched() {
                while (
                        current_ent_id < m_entity_mgr->getMaxEntities()
                        &&
                        !isCurrentEntityMatched()
                      ) 
                {
                    ++current_ent_id;
                }
            }

            Iterator& operator++() {
                ++current_ent_id;
                moveForwardUntilMatched();
                return *this;
            };

    };

    const Iterator begin() const {
        return Iterator(m_entity_mgr, 0, m_component_mask, m_require_all_to_match);
    }

    const Iterator end() const {
        EntityIDNumber index = m_entity_mgr->getMaxEntities();
        return Iterator(m_entity_mgr, index, m_component_mask, m_require_all_to_match);
    }
};


template <typename... Types>
class EntityViewByTypes : public EntityView {
public:

    EntityViewByTypes(
            EntityManager* entity_mgr,
            const bool require_all_to_match = true
            )
        : 
            EntityView(
                    entity_mgr,
                    std::bitset<MAX_COMPONENTS>{},
                    require_all_to_match
                    )
    {
        std::bitset<MAX_COMPONENTS> component_mask;
        component_mask.reset();

        if (sizeof...(Types) == 0) {
            component_mask.set();
        } else 
        {
            // Unpack the template parameters into an initializer list
            std::array<size_t, sizeof...(Types)> component_ids = { entity_mgr->getComponentIndex<Types>() ... };
            for (int i = 0; i < sizeof...(Types); i++) {
                std::cout << "++> i:" << i << " " << component_ids[i] << std::endl;
                component_mask.set(component_ids[i]);
            }
        }
        
        // set m_component_mask
        this->setComponentMask(component_mask);
    }
};


class ISystem {
public:
    virtual Result<VoidResult, GameError> Initialize(Game* game, EntityManager* entity_mgr) { return ResultOK; };
    virtual Result<VoidResult, GameError> Awake(EntityManager* entity_mgr) { return ResultOK; };
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
public:
    PhysicsSystem();

    Result<VoidResult, GameError> Initialize(Game* game, EntityManager* entity_mgr) override;
    Result<VoidResult, GameError> FixedUpdate(double delta_time_s, EntityManager* entity_mgr) override; // Physics update
    Result<VoidResult, GameError> OnComponentRegistered(const ComponentDataPayload* component_info) override;
    Result<VoidResult, GameError> Draw(RenderContext* renderContext, EntityManager* entity_mgr) override;
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
    Result<VoidResult, GameError> OnEntityDestroyed(EntityManager* entity_mgr, EntityID entity_id);
};


#endif
