#include "pixbench/ecs.h"
#include "pixbench/engine_config.h"
#include "pixbench/game.h"
#include "pixbench/physics.h"
#include "pixbench/renderer.h"
#include "pixbench/vector2.h"
#include "SDL3_mixer/SDL_mixer.h"
#include <SDL3/SDL_render.h>
#include <algorithm>
#include <bitset>
#include <cmath>
#include <iostream>
#include <memory>
#include <vector>


ScriptSystem::ScriptSystem() {
    m_script_components_mask.reset();
}


Result<VoidResult, GameError> ScriptSystem::OnComponentRegistered(const ComponentDataPayload* component_info) {
    ComponentTag ctag = component_info->ctag;
    // ComponentType ctype = component_info->ctype;
    size_t cindex = component_info->cindex;

    if (ctag == CTAG_Script) {
        this->m_script_components_mask.set(cindex);
    }

    return ResultOK;
}


Result<VoidResult, GameError> ScriptSystem::OnEntityDestroyed(EntityManager* entity_mgr, EntityID entity_id) {
    for (size_t cindex=0; cindex<MAX_COMPONENTS; ++cindex) {
        if (!(this->m_script_components_mask[cindex]))
            continue; // skip non-script components

        ScriptComponent* script_comp = entity_mgr->getEntityComponentCasted<ScriptComponent>(
                entity_id, cindex);

        if ( !script_comp )
            continue;

        auto res = script_comp->OnDestroy(entity_mgr, entity_id);
        if ( !res.isOk() )
            return res;
    }
    return ResultOK;
}


Result<VoidResult, GameError> ScriptSystem::OnEvent (SDL_Event *event, EntityManager* entity_mgr) {
    // std::cout << "ScriptSystem::OnEvent" << std::endl;
    for (size_t cindex=0; cindex<MAX_COMPONENTS; ++cindex) {
        if (!(this->m_script_components_mask[cindex]))
            continue; // skip non-script components

        // std::cout << "<< cindex=" << cindex << " >>" << std::endl;

        std::bitset<MAX_COMPONENTS> mask;
        mask.reset();
        mask.set(cindex);
        for (auto ent_id : EntityView(entity_mgr, mask, true)) {
            auto script = entity_mgr->getEntityComponentCasted<ScriptComponent>(
                    ent_id, cindex);
            auto res = script->OnEvent(event, entity_mgr, ent_id);
            if ( !res.isOk() ) {
                return res;
            }
        }
    }

    return ResultOK;
}

/*Result<VoidResult, GameError> PreDraw(RenderContext* renderContext, EntityManager* entity_mgr);*/
/*Result<VoidResult, GameError> Draw(RenderContext* renderContext, EntityManager* entity_mgr);*/
Result<VoidResult, GameError> ScriptSystem::Initialize(Game* game, EntityManager* entity_mgr) {
    auto uninitialized_entities = entity_mgr->getUninitializedEntities();
    for (auto& ent_id : uninitialized_entities) {

        for (size_t cindex=0; cindex<MAX_COMPONENTS; ++cindex) {
            if (!(this->m_script_components_mask[cindex]))
                continue; // skip non-script components

            if (!(entity_mgr->isEntityHasComponent(ent_id, cindex))) {
                continue; // skip if entity doesn't have this particular script component
            }

            auto script = entity_mgr->getEntityComponentCasted<ScriptComponent>(
                    ent_id, cindex);
            auto res = script->Init(game, entity_mgr, ent_id);
            if ( !res.isOk() )
                return res;
        }
    }

    return ResultOK;
};

Result<VoidResult, GameError> ScriptSystem::Update(double delta_time_s, EntityManager* entity_mgr) {
    for (size_t cindex=0; cindex<MAX_COMPONENTS; ++cindex) {
        if (!(this->m_script_components_mask[cindex]))
            continue; // skip non-script components

        std::bitset<MAX_COMPONENTS> mask;
        mask.reset();
        mask.set(cindex);
        for (auto ent_id : EntityView(entity_mgr, mask, false)) {
            auto script = entity_mgr->getEntityComponentCasted<ScriptComponent>(
                    ent_id, cindex);
            // std::cout << "ScriptSystem::Update => script->Update(id=" << ent_id.id << ")" << std::endl;
            auto res = script->Update(delta_time_s, entity_mgr, ent_id);
            if ( !res.isOk() )
                return res;
        }
    }

    return ResultOK;
};


Result<VoidResult, GameError> ScriptSystem::LateUpdate(double delta_time_s, EntityManager* entity_mgr) {
    for (size_t cindex=0; cindex<MAX_COMPONENTS; ++cindex) {
        if (!(this->m_script_components_mask[cindex]))
            continue; // skip non-script components

        std::bitset<MAX_COMPONENTS> mask;
        mask.reset();
        mask.set(cindex);
        for (auto ent_id : EntityView(entity_mgr, mask, false)) {
            auto script = entity_mgr->getEntityComponentCasted<ScriptComponent>(
                    ent_id, cindex);
            auto res = script->LateUpdate(delta_time_s, entity_mgr, ent_id);
            if ( !res.isOk() )
                return res;
        }
    }

    return ResultOK;
};


// ===================== Rendering System =====================

Result<VoidResult, GameError> RenderingSystem::OnComponentRegistered(const ComponentDataPayload* component_info) {
    ComponentTag ctag = component_info->ctag;
    size_t cindex = component_info->cindex;

    if (ctag == CTAG_Renderable) {
        this->m_renderable_components_mask.set(cindex);
    }

    return ResultOK;
}


Result<VoidResult, GameError> RenderingSystem::Initialize(Game* game, EntityManager* entity_mgr) {
    std::cout << "RenderingSystem::Initialize" << std::endl;
    auto uninitialized_entities = entity_mgr->getUninitializedEntities();
    for (auto& ent_id : uninitialized_entities) {

        for (size_t cindex=0; cindex<MAX_COMPONENTS; ++cindex) {
            if (!(this->m_renderable_components_mask[cindex]))
                continue; // skip non-script components

            if (!(entity_mgr->isEntityHasComponent(ent_id, cindex))) {
                continue; // skip if entity doesn't have this particular renderable component
            }

            RenderableComponent* renderable = entity_mgr->getEntityComponentCasted<RenderableComponent>(
                    ent_id, cindex);
            Transform* transform = entity_mgr->getEntityComponent<Transform>(ent_id);
            renderable->transform = transform;
        }
    }
    std::cout << "RenderingSystem::Initialize::END" << std::endl;

    return ResultOK;
}


Result<VoidResult, GameError> RenderingSystem::LateUpdate(double delta_time_s, EntityManager* entity_mgr) {
    // Performing animation update (moving the srect)
    std::cout << "RenderingSystem::LateUpdate" << std::endl;

    for (EntityID ent_id : EntityView(entity_mgr, m_renderable_components_mask, false)) {
        if ( !(entity_mgr->isEntityHasComponent<Transform>(ent_id)) )
            continue;

        for (size_t cindex=0; cindex<MAX_COMPONENTS; ++cindex) {
            if (!(this->m_renderable_components_mask[cindex]))
                continue; // skip non-renderable components

            Transform* transform = entity_mgr->getEntityComponent<Transform>(ent_id);
            if (!transform)
                continue;

            RenderableComponent* renderable = entity_mgr->getEntityComponentCasted<RenderableComponent>(
                    ent_id, cindex);

            if (!renderable)
                continue;

            renderable->transform = transform;

            if (renderable->getRenderableTag() == RCTAG_Sprite) {
                Sprite* sprite = static_cast<Sprite*>(renderable);

                if ( !sprite )
                    continue;

                if ( sprite->paused )
                    continue;

                auto sprite_anim = sprite->GetSpriteAnimation().lock();
                if ( !sprite_anim ) {
                    std::cout << "ENT" << ent_id.id << " DOESN'T HAVE SPRITE ANIM" << std::endl;
                    continue;
                }

                sprite->is_animation_finished = false;

                sprite->current_anim_time_s += delta_time_s;
                const double animation_speed = (double)sprite_anim->animation_speed * sprite->animation_speed_multiplier;
                const int elapsed_frames = std::floor(sprite->current_anim_time_s * animation_speed);
                const double time_residue = std::fmod(sprite->current_anim_time_s, 1.0/animation_speed);
                sprite->current_anim_time_s = time_residue;
                sprite->current_frame += elapsed_frames;

                const int max_frame = sprite->maxFrame();
                if (sprite->current_frame >= max_frame) {
                    sprite->is_animation_finished = true;
                }
                if (sprite_anim->repeat) {
                    sprite->current_frame = std::fmod(
                            sprite->current_frame,
                            max_frame
                            );
                } else {
                    sprite->current_frame = std::min(sprite->current_frame, max_frame-1);
                }

                sprite->srect = sprite_anim->res_sheet->GetRectByFrameIndex(sprite->current_frame);
            }
            else if (renderable->getRenderableTag() == RCTAG_Tile) {
                // loop over tile
                Tile* tile = static_cast<Tile*>(renderable);
                if ( !tile )
                    continue;

                auto tile_map = tile->getTileMap().lock();
                if ( !tile_map )
                    continue;

                for (unsigned int i=0; i < tile_map->tile_counts; ++i) {
                    TileAnimation* tile_anim = tile_map->getTileAnimation(i);
                    if ( !tile_anim )
                        continue;

                    tile_anim->advanceFrameByTime(delta_time_s * 1000.0);
                }
            }
        }
    }

    std::cout << "RenderingSystem::Update::END" << std::endl;

    return ResultOK;
};


Result<VoidResult, GameError> RenderingSystem::PreDraw(RenderContext* renderContext, EntityManager* entity_mgr) {
    std::cout << "RenderingSystem::PreDraw" << std::endl;
    ordered_renderables.clear();
    for (EntityID ent_id : EntityView(entity_mgr, m_renderable_components_mask, false)) {
        if ( !(entity_mgr->isEntityHasComponent<Transform>(ent_id)) )
            continue;

        for (size_t cindex=0; cindex<MAX_COMPONENTS; ++cindex) {
            if (!(this->m_renderable_components_mask[cindex]))
                continue; // skip non-script components

            Transform* transform = entity_mgr->getEntityComponent<Transform>(ent_id);
            if (!transform)
                continue;

            RenderableComponent* renderable = entity_mgr->getEntityComponentCasted<RenderableComponent>(
                    ent_id, cindex);

            if (!renderable)
                continue;

            renderable->transform = transform;

            // visibility check
            if (renderable->getRenderableTag() == RCTAG_Sprite) {
                // check if sprite rect inside the screen
                Sprite* sprite = static_cast<Sprite*>(renderable);
                const Vector2 sprite_position__scn = Vector2(
                        sprite->offset.x + sprite->transform->GlobalPosition().x,
                        sprite->offset.y + sprite->transform->GlobalPosition().y
                        );
                const Vector2 sprite_position__cam = sceneToCamSpace(
                        renderContext,
                        sprite_position__scn
                        );
                sprite->drect.x = sprite_position__cam.x;
                sprite->drect.y = sprite_position__cam.y;

                const SDL_FRect r = sprite->drect;
                const float cam_w = renderContext->camera_size.x;
                const float cam_h = renderContext->camera_size.y;
                if (
                        !(
                            std::max(r.x+r.w, cam_w) - std::min(r.x, 0.0f) < (cam_w+r.w)
                            &&
                            std::max(r.y+r.h, cam_h) - std::min(r.y, 0.0f) < (cam_h+r.h)
                         )
                   ) { // skip if not visible
                    continue;
                }
            }
            else if (renderable->getRenderableTag() == RCTAG_Tile) {
                Tile* tile = static_cast<Tile*>(renderable);
                if (!tile)
                    continue;

                if (auto tile_map = tile->getTileMap().lock()) {
                    const Vector2 tile_pos = tile->transform->GlobalPosition();
                    const Vector2 tile_pos__cam = sceneToCamSpace(renderContext, tile_pos);
                    tile->drect.x = tile_pos__cam.x;
                    tile->drect.y = tile_pos__cam.y;
                    tile->drect.w = tile_map->width;
                    tile->drect.h = tile_map->height;
                    const SDL_FRect r = tile->drect;

                    const float cam_w = renderContext->camera_size.x;
                    const float cam_h = renderContext->camera_size.y;

                    if (
                            !(
                                std::max(r.x+r.w, cam_w) - std::min(r.x, 0.0f) < (cam_w+r.w)
                                &&
                                std::max(r.y+r.h, cam_h) - std::min(r.y, 0.0f) < (cam_h+r.h)
                             )
                       ) { // skip if not visible
                        continue;
                    }

                }
            }
            else if (renderable->getRenderableTag() == RCTAG_Script) {
                // TODO: check if drawable script rect inside the screen
                CustomRenderable* custom_renderable = static_cast<CustomRenderable*>(renderable);
                const Vector2 render_position__scn = Vector2(
                        custom_renderable->offset.x + custom_renderable->transform->GlobalPosition().x,
                        custom_renderable->offset.y + custom_renderable->transform->GlobalPosition().y
                        );
                const Vector2 render_position__cam = sceneToCamSpace(
                        renderContext,
                        render_position__scn
                        );
                custom_renderable->drect.x = render_position__cam.x;
                custom_renderable->drect.y = render_position__cam.y;

                const SDL_FRect r = custom_renderable->drect;
                const float cam_w = renderContext->camera_size.x;
                const float cam_h = renderContext->camera_size.y;
                if (
                        !(
                            std::max(r.x+r.w, cam_w) - std::min(r.x, 0.0f) < (cam_w+r.w)
                            &&
                            std::max(r.y+r.h, cam_h) - std::min(r.y, 0.0f) < (cam_h+r.h)
                         )
                   ) { // skip if not visible
                    continue;
                }
            }

            ordered_renderables.push_back(renderable);
        }
    }

    // Calculate render order by depth (higher is the closer forward)
    std::sort(
            ordered_renderables.begin(),
            ordered_renderables.end(),
            [] (const RenderableComponent* renderable_a, const RenderableComponent* renderable_b) {
                return renderable_a->depth < renderable_b->depth;
            }
            );
    std::cout << "RenderingSystem::PreDraw::END" << std::endl;

    return ResultOK;
}


Result<VoidResult, GameError> RenderingSystem::Draw(RenderContext* renderContext, EntityManager* entity_mgr) {
    std::cout << "RenderingSystem::Draw" << std::endl;

    for (RenderableComponent* renderable : ordered_renderables) {
        if (renderable->getRenderableTag() == RCTAG_Sprite) {
            Sprite* sprite = static_cast<Sprite*>(renderable);
            const SDL_FRect sprite_drect__scr = camToScreenSpace(renderContext, sprite->drect);
            bool err = SDL_RenderTextureRotated(
                    renderContext->renderer,
                    sprite->texture->texture,
                    &(sprite->srect), &(sprite_drect__scr),
                    0,
                    nullptr,
                    sprite->flip_mode
                    );
            if (sprite->srect.w == 0 || sprite->srect.h == 0) {
                std::cout << "<< ERROR >> sprite->srect w & h = 0" << std::endl;
            }
            if (!err) {
                std::cout << "<< ERROR >> problem in rendering sprite" << std::endl;
                std::cout << "SDL error:" << std::endl << SDL_GetError() << std::endl;
            }
        }
        else if (renderable->getRenderableTag() == RCTAG_Script) {
            CustomRenderable* custom_renderable = static_cast<CustomRenderable*>(renderable);
            custom_renderable->Draw(renderContext, entity_mgr);
        }
        else if (renderable->getRenderableTag() == RCTAG_Tile) {
            // Find the ranges of tiles in the map to render

            Tile* tile = static_cast<Tile*>(renderable);

            auto tile_map = tile->getTileMap().lock();
            if (!tile_map)
                continue;

            auto atlass = tile_map->getAtlass().lock();
            if (!atlass)
                continue;

            // we're still working on scene space
            const SDL_FRect r = tile->drect;
            const auto cam_size = renderContext->camera_size;
            const int start_col = std::floor(std::max(0.0f, -r.x) / tile_map->tile_w);
            const int start_row = std::floor(std::max(0.0f, -r.y) / tile_map->tile_h);
            const int end_col = tile_map->columns - std::floor(
                    std::max(0.0f, (r.x+tile_map->width - cam_size.x)) /
                    tile_map->tile_w
                    );
            const int end_row = tile_map->rows - std::floor(
                    std::max(0.0f, (r.y+tile_map->height - cam_size.y)) /
                    tile_map->tile_h
                    );

            // Perform for loop to draw tiles
            // this first pass is only for drawing non animated tiles
            SDL_FRect srect, drect;
            drect.w = tile_map->tile_w;
            drect.h = tile_map->tile_h;
            drect = camToScreenSpace(renderContext, drect);     // scaling the size
            for (int c=start_col; c<end_col; ++c) {
                for (int r=start_row; r<end_row; ++r) {

                    int tile_id = tile_map->getTileIDbyTilePosition(r, c);
                    if (tile_id == 0)
                        continue;

                    srect = atlass->getRectByIndex(tile_id-1);
                    const Vector2 this_tile_pos__scr = 
                        camToScreenSpace(
                                renderContext,
                                Vector2(
                                    tile->drect.x + c * tile_map->tile_w,
                                    tile->drect.y + r * tile_map->tile_h
                                )
                                );
                    drect.x = this_tile_pos__scr.x;
                    drect.y = this_tile_pos__scr.y;

                    SDL_RenderTexture(
                            renderContext->renderer,
                            atlass->getTexture()->texture,
                            &srect, &drect
                            );
                }
            }

            // this the second pass for Tile,
            // this one is for drawing animated tiles.
            for (int c=start_col; c<end_col; ++c) {
                for (int r=start_row; r<end_row; ++r) {
                    unsigned int anim_tile_id = tile_map->getTileIDbyTilePosition(r, c);
                    if (anim_tile_id == 0)
                        continue;

                    TileAnimation* tile_anim = tile_map->getTileAnimation(anim_tile_id);
                    if (!tile_anim)
                        continue;

                    unsigned int tile_id = tile_anim->getCurrentFrame()->tile_id;
                    srect = atlass->getRectByIndex(tile_id-1);
                    drect.x = tile->drect.x + c * tile_map->tile_w;
                    drect.y = tile->drect.y + r * tile_map->tile_h;

                    // drect = camToScreenSpace(renderContext, drect);

                    SDL_RenderTexture(
                            renderContext->renderer,
                            atlass->getTexture()->texture,
                            &srect, &drect
                            );
                }
            }

        }
    }
    std::cout << "RenderingSystem::Draw::END" << std::endl;

    return ResultOK;
}


// ===================== Audio System =====================


Result<VoidResult, GameError> AudioSystem::Initialize(Game* game, EntityManager* entity_mgr) {
    std::cout << "AudioSystem::Initialize::BEGIN" << std::endl;
    // initialized states
    m_channel_states.resize(
            game->audioContext->numAvailableChannel()
            );

    std::cout << "AudioSystem::Initialize::END" << std::endl;
    return ResultOK;
}


Result<VoidResult, GameError> AudioSystem::LateUpdate(double delta_time_s, EntityManager* entity_mgr) {
    std::cout << "AudioSystem::LateUpdate::BEGINEN" << std::endl;

    EntityID need_sync_entities[MAX_ENTITIES];
    int need_sync_entities_len = 0;

    // update all audio play states
    // gather audio_player that need to be synched
    for (EntityID ent_id : EntityViewByTypes<AudioPlayer>(entity_mgr)) {
        if ( !(entity_mgr->isEntityHasComponent<AudioPlayer>(ent_id)) )
            continue;

        AudioPlayer* audio_player = entity_mgr->getEntityComponent<AudioPlayer>(ent_id);
        if ( !audio_player )
            continue;

        if ( !audio_player->__isNeedSync() )
            continue;

        need_sync_entities[need_sync_entities_len] = ent_id;
        need_sync_entities_len++;
    }

    // sync volume, etc.
    for (int i = 0; i < need_sync_entities_len; ++i) {
        const EntityID ent_id = need_sync_entities[i];
        AudioPlayer* audio_player = entity_mgr->getEntityComponent<AudioPlayer>(ent_id);

        const int channel = audio_player->getAssignedChannel();
        if ( channel == -1 )
            continue;

        // change volume
        if ( this->m_channel_states[channel].volume != audio_player->volume ) {
            Mix_Volume(channel, audio_player->volume);
            this->m_channel_states[channel].volume = audio_player->volume;
        }

        // paused what need to be paused
        if ( this->m_channel_states[channel].is_paused != audio_player->isPaused() ) {
            if ( audio_player->isPaused() ) {
                Mix_Pause(channel);
            }
            else {
                Mix_Resume(channel);
            }
            this->m_channel_states[channel].is_paused = audio_player->isPaused();
        }

        // stop what need to be stopped, to obtained all available channels
        if ( this->m_channel_states[channel].is_playing != audio_player->isPlaying() && !audio_player->isPlaying() ) {
            Mix_HaltChannel(channel);
            this->m_channel_states[channel].is_playing = false;
            this->m_channel_states[channel].is_active = false;
            this->m_channel_states[channel].clip = nullptr;
            this->m_channel_states[channel].position = 0.0;
            this->m_channel_states[channel].channel = -1;

            audio_player->__setAssignedChannel(-1);
            audio_player->__setIsFinished(true);
        }

        // play what needs to be replayed
        if ( audio_player->__isNeedToReplay() && audio_player->getAssignedChannel() != -1 ) {
            int loop_param = 0;
            if (audio_player->is_looping) {
                loop_param = -1;
            }
            Mix_PlayChannel(
                    audio_player->getAssignedChannel(),
                    audio_player->clip->chunk,
                    loop_param
                    );
        }
    }

    // aligning play state
    for (auto ent_id : EntityViewByTypes<AudioPlayer>(entity_mgr)) {
        AudioPlayer* audio_player = entity_mgr->getEntityComponent<AudioPlayer>(ent_id);

        int assigned_channel = audio_player->getAssignedChannel();
        if ( assigned_channel == -1 )
            continue;

        bool is_channel_playing = Mix_Playing(assigned_channel);
        if ( !is_channel_playing ) {
            m_channel_states[assigned_channel].is_playing = is_channel_playing;
            audio_player->__setIsPlaying(is_channel_playing);
            audio_player->__setAssignedChannel(-1);
        }
    }

    // play what need to be played
    for (int i = 0; i < need_sync_entities_len; ++i) {
        const EntityID ent_id = need_sync_entities[i];
        AudioPlayer* audio_player = entity_mgr->getEntityComponent<AudioPlayer>(ent_id);

        if ( audio_player->isPlaying() && audio_player->getAssignedChannel() == -1 ) {
            int loop_param = 0;
            if (audio_player->is_looping) {
                loop_param = -1;
            }

            const int play_channel = Mix_PlayChannel(-1, audio_player->clip->chunk, loop_param);
            audio_player->__setAssignedChannel(play_channel);

            if (play_channel != -1) {
                m_channel_states[play_channel].is_active = true;
                m_channel_states[play_channel].clip = audio_player->clip;
                m_channel_states[play_channel].channel = play_channel;
                m_channel_states[play_channel].is_playing = true;
                m_channel_states[play_channel].is_paused = false;
                m_channel_states[play_channel].position = 0.0;
                m_channel_states[play_channel].volume = audio_player->volume;

                Mix_Volume(play_channel, audio_player->volume);
            }
        }
    }

    for (int i = 0; i < need_sync_entities_len; ++i) {
        const EntityID ent_id = need_sync_entities[i];
        AudioPlayer* audio_player = entity_mgr->getEntityComponent<AudioPlayer>(ent_id);

        audio_player->__resetNeedSyncStatus();
    }

    std::cout << "AudioSystem::LateUpdate::END" << std::endl;
    return ResultOK;
}



// ===================== Physics System =====================


PhysicsSystem::PhysicsSystem() {
    m_physics_components_mask.reset();
}


Result<VoidResult, GameError> PhysicsSystem::Initialize(Game* game, EntityManager* entity_mgr) {
    return ResultOK;
}


Result<VoidResult, GameError> PhysicsSystem::OnComponentRegistered(const ComponentDataPayload* component_info) {
    ComponentTag ctag = component_info->ctag;
    size_t cindex = component_info->cindex;

    if (ctag == CTAG_Collider) {
        this->m_physics_components_mask.set(cindex);
    }

    return ResultOK;
}


Result<VoidResult, GameError> PhysicsSystem::FixedUpdate(double delta_time_s, EntityManager* entity_mgr) {

    struct ColliderObject {
        EntityID entity;
        Collider* collider = nullptr;
        ColliderTag collider_tag;
    };
    
    std::vector<ColliderObject> colliders;
    colliders.reserve(MAX_ENTITIES); // set capacity to at least contains all entities
    
    // list all colliders
    for (size_t cindex=0; cindex<MAX_COMPONENTS; ++cindex) {
        if (!(this->m_physics_components_mask[cindex]))
            continue; // skip non-collider components

        std::bitset<MAX_COMPONENTS> mask;
        mask.reset();
        mask.set(cindex);
        for (auto ent_id : EntityView(entity_mgr, mask, false)) {
            auto collider = entity_mgr->getEntityComponentCasted<Collider>(
                    ent_id, cindex);
            auto transform = entity_mgr->getEntityComponent<Transform>(
                    ent_id);
            if ( !transform )
                continue;

            collider->__transform.SetPosition(transform->GlobalPosition());
            collider->__transform.rotation = transform->rotation;
            ColliderObject collider_object;
            collider_object.entity = ent_id;
            collider_object.collider = collider;
            collider_object.collider_tag = collider->getColliderTag();
            colliders.push_back(collider_object);
        }
    }
    
    // narrow phase: pair checks
    // for now, we'll only gonna do naive every pair checks
    int ind = 0;
    for (auto& coll_1 : colliders) {
        for (auto coll_2 = colliders.begin() + ind; coll_2 != colliders.end(); ++coll_2) {
            if ( coll_1.entity.id == coll_2->entity.id )
                continue;

            // pair checking
            CollisionManifold manifold = CollisionManifold(Vector2::RIGHT, 0.0);
            manifold.point_count = 0;
            bool is_body_1_the_ref = false;
            bool is_colliding = true;
            switch (coll_1.collider_tag) {
                case COLTAG_Box:
                    switch (coll_2->collider_tag) {
                        case COLTAG_Box:
                            is_colliding = boxToBoxCollision(
                                    static_cast<BoxCollider*>(coll_1.collider),
                                    static_cast<BoxCollider*>(coll_2->collider),
                                    &manifold,
                                    &is_body_1_the_ref
                                    );
                            break;
                        case COLTAG_Circle:
                            is_colliding = boxToCircleCollision(
                                    static_cast<BoxCollider*>(coll_1.collider),
                                    static_cast<CircleCollider*>(coll_2->collider),
                                    &manifold,
                                    &is_body_1_the_ref
                                    );
                            break;
                        case COLTAG_Polygon:
                            is_colliding = boxToPolygonCollision(
                                    static_cast<BoxCollider*>(coll_1.collider),
                                    static_cast<PolygonCollider*>(coll_2->collider),
                                    &manifold,
                                    &is_body_1_the_ref
                                    );
                            break;
                    }
                    break;
                case COLTAG_Circle:
                    switch (coll_2->collider_tag) {
                        case COLTAG_Box:
                            is_colliding = boxToCircleCollision(
                                    static_cast<BoxCollider*>(coll_2->collider),
                                    static_cast<CircleCollider*>(coll_1.collider),
                                    &manifold,
                                    &is_body_1_the_ref
                                    );
                            break;
                        case COLTAG_Circle:
                            is_colliding = circleToCircleCollision(
                                    static_cast<CircleCollider*>(coll_1.collider),
                                    static_cast<CircleCollider*>(coll_2->collider),
                                    &manifold,
                                    &is_body_1_the_ref
                                    );
                            break;
                        case COLTAG_Polygon:
                            is_colliding = circleToPolygonCollision(
                                    static_cast<CircleCollider*>(coll_1.collider),
                                    static_cast<PolygonCollider*>(coll_2->collider),
                                    &manifold,
                                    &is_body_1_the_ref
                                    );
                            break;
                    }
                    break;
                case COLTAG_Polygon:
                    switch (coll_2->collider_tag) {
                        case COLTAG_Box:
                            is_colliding = boxToPolygonCollision(
                                    static_cast<BoxCollider*>(coll_2->collider),
                                    static_cast<PolygonCollider*>(coll_1.collider),
                                    &manifold,
                                    &is_body_1_the_ref
                                    );
                            break;
                        case COLTAG_Circle:
                            is_colliding = circleToPolygonCollision(
                                    static_cast<CircleCollider*>(coll_2->collider),
                                    static_cast<PolygonCollider*>(coll_1.collider),
                                    &manifold,
                                    &is_body_1_the_ref
                                    );
                            break;
                        case COLTAG_Polygon:
                            is_colliding = polygonToPolygonCollision(
                                    static_cast<PolygonCollider*>(coll_1.collider),
                                    static_cast<PolygonCollider*>(coll_2->collider),
                                    &manifold,
                                    &is_body_1_the_ref
                                    );
                            break;
                    }
                    break;
            }

            // TODO: handle collision
            // the returned manifold is in the perspective of the reference body
            // determined the reference body using `is_body_1_the_ref`

            // no manifold points means no collision
            if ( is_colliding ) {
                bool trigger_enter = false;
                // if originally no collision happening between this 2 object
                if ( coll_1.collider->getManifold().point_count == 0 )
                    trigger_enter = true;

                coll_1.collider->setManifold(manifold);
                coll_2->collider->setManifold(manifold);

                if ( is_body_1_the_ref )
                    coll_2->collider->getManifold().flipNormal();
                else
                    coll_1.collider->getManifold().flipNormal();

                if ( trigger_enter ) {
                    coll_1.collider->__triggerOnBodyEnter();
                    coll_2->collider->__triggerOnBodyEnter();
                }
            }
            // manifold points means there's collision
            else {
                // if originally no collision happening between this 2 object
                if ( coll_1.collider->getManifold().point_count == 0 )
                    continue;

                coll_1.collider->setManifold(manifold);
                coll_2->collider->setManifold(manifold);
                
                coll_1.collider->__triggerOnBodyLeave();
                coll_2->collider->__triggerOnBodyLeave();
            }
        }
        
        ++ind;
    }

    return ResultOK;
}


Result<VoidResult, GameError> PhysicsSystem::Draw(RenderContext* renderContext, EntityManager* entity_mgr) {

#ifdef PHYSICS_DEBUG_DRAW

    // draw 
    for (size_t cindex=0; cindex<MAX_COMPONENTS; ++cindex) {
        if (!(this->m_physics_components_mask[cindex]))
            continue; // skip non-collider components

        std::bitset<MAX_COMPONENTS> mask;
        mask.reset();
        mask.set(cindex);
        
        for (auto ent_id : EntityView(entity_mgr, mask, false)) {
            Collider* coll = entity_mgr->getEntityComponentCasted<Collider>(ent_id, cindex);
            if ( !coll )
                continue;

            Transform* transform = entity_mgr->getEntityComponent<Transform>(ent_id);
            if ( !transform )
                continue;

            switch ( coll->getColliderTag() ) {
                case COLTAG_Box: 
                    {
                    BoxCollider* box_coll = static_cast<BoxCollider*>(coll);
                    if ( box_coll->getManifold().point_count > 0 ) {
                        SDL_SetRenderDrawColorFloat(
                                renderContext->renderer,
                                1.0, 0.0, 0.0, 1.0
                                );
                    }
                    else {
                        SDL_SetRenderDrawColorFloat(
                                renderContext->renderer,
                                0.0, 1.0, 0.0, 1.0
                                );
                    }
                    SDL_FPoint points[5];
                    for (int i=0; i<4; ++i) {
                        const Vector2 vert = box_coll->__polygon.getVertex(
                                i,
                                box_coll->__transform.GlobalPosition(),
                                box_coll->__transform.rotation
                                );
                        points[i] = { vert.x, vert.y };
                        if ( i == 0 ) {
                            points[4] = { vert.x, vert.y };
                        }
                    }
                    SDL_RenderLines(
                            renderContext->renderer,
                            points,
                            5);
                    // manifold
                    CollisionManifold& manifold = box_coll->getManifold();
                    for (int i=0; i<manifold.point_count; ++i) {
                        Vector2 contact = manifold.points[i];

                        SDL_SetRenderDrawColorFloat(
                                renderContext->renderer,
                                0.0, 0.0, 1.0, 1.0
                                );
                        phydebDrawCross(
                                renderContext->renderer,
                                &contact
                                );
                    }

                    break;
                    }
                case COLTAG_Circle:
                    {
                    break;
                    }
                case COLTAG_Polygon:
                    {
                    PolygonCollider* poly_coll = static_cast<PolygonCollider*>(coll);
                    if ( poly_coll->getManifold().point_count > 0 ) {
                        SDL_SetRenderDrawColorFloat(
                                renderContext->renderer,
                                1.0, 0.0, 0.0, 1.0
                                );
                    }
                    else {
                        SDL_SetRenderDrawColorFloat(
                                renderContext->renderer,
                                0.0, 1.0, 0.0, 1.0
                                );
                    }
                    SDL_FPoint points[MAX_POLYGON_VERTEX+1];
                    for (int i=0; i<poly_coll->__polygon.vertex_counts; ++i) {
                        const Vector2 vert = poly_coll->__polygon.getVertex(
                                i,
                                poly_coll->__transform.GlobalPosition(),
                                poly_coll->__transform.rotation
                                );
                        points[i] = { vert.x, vert.y };
                        if ( i == 0 ) {
                            points[poly_coll->__polygon.vertex_counts] = { vert.x, vert.y };
                        }
                    }
                    SDL_RenderLines(
                            renderContext->renderer,
                            points,
                            poly_coll->__polygon.vertex_counts+1);
                    // manifold
                    CollisionManifold& manifold = poly_coll->getManifold();
                    for (int i=0; i<manifold.point_count; ++i) {
                        Vector2 contact = manifold.points[i];

                        SDL_SetRenderDrawColorFloat(
                                renderContext->renderer,
                                0.0, 0.0, 1.0, 1.0
                                );
                        phydebDrawCross(
                                renderContext->renderer,
                                &contact
                                );
                        // normals
                        SDL_SetRenderDrawColorFloat(
                                renderContext->renderer,
                                0.0, 1.0, 0.5, 1.0);
                        SDL_RenderLine(
                                renderContext->renderer,
                                contact.x, contact.y,
                                contact.x + manifold.normal.x * 32.0,
                                contact.y + manifold.normal.y * 32.0
                                );
                    }

                    break;
                    }

            }
        }
    }

    return ResultOK;
#endif

    return ResultOK;
}
