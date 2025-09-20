#ifndef COMPONENTS_HEADER
#define COMPONENTS_HEADER


#include "pixbench/audio.h"
#include "pixbench/entity.h"
#include "pixbench/physics/type.h"
#include "pixbench/utils/results.h"
#include "pixbench/renderer.h"
#include "pixbench/vector2.h"
#include <functional>
#include <random>
#include <string>


// ========== FORWARD DECLARATION ========== BEGIN
class Game;

/*
 * ! special case of forward declaration for system. Component idealy shouldn't
 * have access asd
 */
class PhysicsSystem;
// ========== FORWARD DECLARATION ========== END


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

    const Vector2* __globalPosPtr() { return &globalPosition; }
    const Vector2* __localPosPtr() { return &localPosition; }
};


enum ColliderTag {
    COLTAG_Box,
    COLTAG_Circle,
    COLTAG_Capsule,
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

class Collider : public IComponent {
private:
    std::function<void(CollisionEvent)> m_on_body_enter_callback = nullptr;
    std::function<void(EntityID)> m_on_body_leave_callback = nullptr;
    CollisionManifold m_manifold;
    EntityID m_entity;
public:
    bool is_static = true;              //!< static-to-static collision will be ignored
    float skin_depth = 1.0;             //!< skin depth for collision detection
    float __bounding_radius = 1.0;
    Transform __transform = Transform();
    PhysicsSystem* __physics_system{ nullptr };

    void __setEntity(EntityID entity) {
        m_entity = entity;
    }

    /*
     * The entity this collider is attached to
     */
    EntityID entity() {
        return m_entity;
    }

    virtual ColliderTag getColliderTag() const {
        return COLTAG_Box;
    };


    /*
     * Set callbacks to call when this collider begins overlap with other collider
     */
    virtual void setOnBodyEnterCallback(
            std::function<void(CollisionEvent)> callback_function
            ) {
        m_on_body_enter_callback = callback_function;
    }

    /*
     * Set callbacks to call when this collider leaves other collider
     */
    virtual void setOnBodyLeaveCallback(
            std::function<void(EntityID)> callback_function
            ) {
        m_on_body_leave_callback = callback_function;
    }

    /*
     * Returns true of this collider is colliding with other collider(s)
     */
    bool isColliding();

    /*
     * Returns list of collision manifolds
     * Note: checks whether this collider is colliding or not by calling `isColliding()`
     */
    std::vector<CollisionEvent> getCollisionState();

    /*
     * Set collider's global position in the game world
     */
    void setPosition(const Vector2 position) {
        this->__transform.SetPosition(position);
    }

    /*
     * Get collider's global position in the game world
     */
    Vector2 getPosition() {
        return this->__transform.GlobalPosition();
    }

    void __triggerOnBodyEnter(EntityID other_id);
    void __triggerOnBodyLeave(EntityID other_id);

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

        __bounding_radius = std::sqrt(this->width*this->width/4.0 + this->height*this->height/4.0);
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

    void setRadius(float r) {
        this->radius = r;
        this->__bounding_radius = r + 2.0; // 2.0 of padding
    }
    
    ColliderTag getColliderTag() const override {
        return COLTAG_Circle;
    };
};


class CapsuleCollider : public Collider {
public:
    float radius = 16.0;            //!< capsule cap radius
    float length = 32.0;            //!< capsule length (outside the radius)

    void setSize(float r, float l) {
        this->radius = r;
        this->length = l;
        this->__bounding_radius = l/2.0 + r + 2.0; // 2.0 of padding
    }

    ColliderTag getColliderTag() const override {
        return COLTAG_Capsule;
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

        // find max radius
        for (size_t i=0; i<__polygon.vertex_counts; ++i) {
            const Vector2 vert = __polygon.vertex[i];
            const float r = std::sqrt(vert.x*vert.x + vert.y*vert.y);
            if (i == 0 || r > __bounding_radius) {
                __bounding_radius = r;
            }
        }

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
    bool is_always_visible{ true };

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

#endif
