#include "pixbench/engine_config.h"
#include "pixbench/components.h"
#include "pixbench/systems.h"
#include "pixbench/ecs.h"
#include "pixbench/entity.h"
#include "pixbench/game.h"
#include "pixbench/physics/physics.h"
#include "pixbench/physics/type.h"
#include "pixbench/renderer.h"
#include "pixbench/utils/results.h"
#include "pixbench/vector2.h"
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>


// ===================== Hierarchy System =====================

#define HIERARCHY_STACK_CHILD_COUNT 4


HierarchySystem::HierarchySystem() {
    this->m_empty_transform.SetLocalPosition(Vector2::ZERO);
    this->m_empty_transform.localRotation = 0.0;
    this->m_empty_transform.__deParent();
}


void HierarchySystem::_addChildToEntity(EntityID parent, EntityID child) {
    Hierarchy* p_hie = m_game->entityManager->getEntityComponent<Hierarchy>(parent);
    Hierarchy* c_hie = m_game->entityManager->getEntityComponent<Hierarchy>(child);

    if ( !p_hie || !c_hie )
        return;

    if ( c_hie->hasParent() )
        return;

    // check if child is already in the entity childs list
    for (size_t i=0; i<p_hie->numChilds(); ++i) {
        EntityID child_ent;
        if (i < HIERARCHY_STACK_CHILD_COUNT) {
            child_ent = p_hie->_array_childs[i];
        } else {
            child_ent = m_child_store[parent.id][i-HIERARCHY_STACK_CHILD_COUNT];
        }

        if (child_ent.id == child.id) {
            if (child.version <= child_ent.version) {
                return;
            }

            if (i < HIERARCHY_STACK_CHILD_COUNT) {
                p_hie->_array_childs[i] = child;
            }
            else {
                m_child_store[parent.id][i-HIERARCHY_STACK_CHILD_COUNT] = child;
            }
        }
    }

    if ( p_hie->numChilds() + 1 > HIERARCHY_STACK_CHILD_COUNT ) {
        if (m_child_store.find(parent.id) == m_child_store.end()) {
            m_child_store[parent.id] = std::vector<EntityID>();
        }
        m_child_store[parent.id].push_back(child);
    }
    else {
        p_hie->_array_childs[p_hie->numChilds()] = child;
    }
    c_hie->_parent = parent;
    c_hie->_has_parent = true;
    p_hie->__incrNumChild();

    // handle child transform
    Transform* child_trans = m_game->entityManager->getEntityComponent<Transform>(child);
    Transform* parent_trans = m_game->entityManager->getEntityComponent<Transform>(parent);
    if ( parent_trans && child_trans ) {
        child_trans->__syncLocalPositionOnParent(parent_trans);
    }
}

void HierarchySystem::_addChildsToEntity(EntityID parent, std::vector<EntityID> childs) {
    assert(false && "Unimplemented function");
}

bool HierarchySystem::_getEntityChildAtIndex(EntityID parent, size_t index, EntityID* out__child_id) {
    Hierarchy* p_hie = m_game->entityManager->getEntityComponent<Hierarchy>(parent);

    if ( !p_hie )
        return false;

    if (index > p_hie->numChilds() - 1) {
        return false;
    }

    if (index < HIERARCHY_STACK_CHILD_COUNT) {
        *out__child_id = p_hie->_array_childs[index];
        return true;
    } else {
        *out__child_id = m_child_store[parent.id][index-HIERARCHY_STACK_CHILD_COUNT];
        return true;
    }

    return true;
}

void HierarchySystem::_removeChildFromEntity(EntityID parent, EntityID child) {
    Hierarchy* p_hie = m_game->entityManager->getEntityComponent<Hierarchy>(parent);
    Hierarchy* c_hie = m_game->entityManager->getEntityComponent<Hierarchy>(child);

    if ( !p_hie || !c_hie )
        return;

    size_t num_childs = p_hie->numChilds();

    for (size_t i=0; i<p_hie->numChilds(); ++i) {
        EntityID child_ent;
        if (i < HIERARCHY_STACK_CHILD_COUNT) {
            child_ent = p_hie->_array_childs[i];
        } else {
            child_ent = m_child_store[parent.id][i - HIERARCHY_STACK_CHILD_COUNT];
        }

        if (child_ent.id == child.id && child_ent.version == child.version) {
            // TODO: remove entity
            EntityID last_child;
            _getEntityChildAtIndex(parent, num_childs-1, &last_child);
            if (i < HIERARCHY_STACK_CHILD_COUNT) {
                p_hie->_array_childs[i] = last_child;
                if (num_childs > HIERARCHY_STACK_CHILD_COUNT) {
                    m_child_store[parent.id].erase(m_child_store[parent.id].begin() + (num_childs - 1 - HIERARCHY_STACK_CHILD_COUNT));
                }
            }
            else {
                m_child_store[parent.id].erase(m_child_store[parent.id].begin() + (i - HIERARCHY_STACK_CHILD_COUNT));
            }

            --num_childs;
        }
    }

    Transform* child_trans = m_game->entityManager->getEntityComponent<Transform>(child);
    if (child_trans) {
        child_trans->__deParent();
    }

    c_hie->_has_parent = false;
    p_hie->__setNumChild(num_childs);
}

size_t HierarchySystem::_getEntityChildCount(EntityID parent) {
    Hierarchy* p_hie = m_game->entityManager->getEntityComponent<Hierarchy>(parent);

    if ( !p_hie )
        return 0;

    return p_hie->numChilds();
}

void HierarchySystem::_getEntityChilds(EntityID parent, std::vector<EntityID>& out__childs, bool recursive) {
    Hierarchy* p_hie = m_game->entityManager->getEntityComponent<Hierarchy>(parent);

    if ( !p_hie )
        return;

    for (size_t i=0; i<p_hie->numChilds(); ++i) {
        EntityID current_child;
        if (i < HIERARCHY_STACK_CHILD_COUNT) {
            current_child = p_hie->_array_childs[i];
        } else {
            current_child = m_child_store[parent.id][i-HIERARCHY_STACK_CHILD_COUNT];
        }

        out__childs.push_back(current_child);
        if (recursive) {
            _getEntityChilds(current_child, out__childs, true);
        }
    }
}


Result<VoidResult, GameError> HierarchySystem::Initialize(Game* game, EntityManager* entity_mgr) {

    for (EntityID ent_id : EntityViewByTypes<Hierarchy>(entity_mgr)) {
        Hierarchy* hierarchy = entity_mgr->getEntityComponent<Hierarchy>(ent_id);
        hierarchy->_self = ent_id;
    }

    return ResultOK;
}


Result<VoidResult, GameError> HierarchySystem::FixedUpdate(double delta_time_s, EntityManager* entity_mgr) {

    this->_syncAllTransform();

    return ResultOK;
}


void HierarchySystem::_entitySyncing(EntityID parent, Hierarchy* parent_hierarchy, Transform* last_parent_transform) {
    EntityManager* ent_mgr = m_game->entityManager;

    Transform* ent_trans = m_game->entityManager->getEntityComponent<Transform>(parent);

    if ( !last_parent_transform ) {
        last_parent_transform = &(this->m_empty_transform);
    }

    if ( ent_trans ) {
        ent_trans->syncGlobalFromLocalBasedOnParent(*last_parent_transform);
        last_parent_transform = ent_trans;
    }

    std::vector<EntityID> childs;
    this->_getEntityChilds(parent, childs);

    for (EntityID ent : childs) {
        Hierarchy* child_hie = ent_mgr->getEntityComponent<Hierarchy>(ent);
        _entitySyncing(ent, child_hie, last_parent_transform);
    }
}


void HierarchySystem::_syncEntityTransform(EntityID parent) {
    assert(m_game && "HierarchySystem::m_game shouldn't be null.");

    EntityManager* ent_mgr = m_game->entityManager;
    assert(ent_mgr && "Game::entityManager shouldn't be null.");

    Hierarchy* p_hie = ent_mgr->getEntityComponent<Hierarchy>(parent);
    assert(p_hie && "`parent` doesn't have component `Hierarchy`.");

    Transform* p_trans = nullptr;
    if (p_hie->hasParent()) {
        Transform* pp_trans = m_game->entityManager->getEntityComponent<Transform>(p_hie->parent());
        if (pp_trans)
            p_trans = pp_trans;
    }

    // recursively traverse down the hierarchy tree
    _entitySyncing(parent, p_hie, p_trans);
}


void HierarchySystem::_syncAllTransform() {
    assert(m_game && "HierarchySystem::m_game shouldn't be null.");

    EntityManager* ent_mgr = m_game->entityManager;
    assert(ent_mgr && "Game::entityManager shouldn't be null.");

    for (EntityID ent_id : EntityViewByTypes<Hierarchy>(ent_mgr)) {
        Hierarchy* ent_hierarchy = ent_mgr->getEntityComponent<Hierarchy>(ent_id);
        if ( !ent_hierarchy )
            continue;

        if ( ent_hierarchy->hasParent() ) {
            continue;
        }

        this->_entitySyncing(ent_id, ent_hierarchy, nullptr);
    }
}


// ===================== Hierarchy System =====================


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


Result<VoidResult, GameError> ScriptSystem::FixedUpdate(double delta_time_s, EntityManager* entity_mgr) {
    for (size_t cindex=0; cindex<MAX_COMPONENTS; ++cindex) {
        if (!(this->m_script_components_mask[cindex]))
            continue; // skip non-script components

        std::bitset<MAX_COMPONENTS> mask;
        mask.reset();
        mask.set(cindex);
        for (auto ent_id : EntityView(entity_mgr, mask, false)) {
            auto script = entity_mgr->getEntityComponentCasted<ScriptComponent>(
                    ent_id, cindex);
            auto res = script->FixedUpdate(delta_time_s, entity_mgr, ent_id);
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

                if ( custom_renderable->is_always_visible ) {
                    ordered_renderables.push_back(renderable);
                    continue;
                }

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
                    180 * sprite->transform->rotation / M_PI,
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
    for (size_t i=0; i<MAX_ENTITIES; ++i) {
        m_collisions_count[i] = 0;
    }
}


bool PhysicsSystem::isPairColliding(EntityID ent_a, EntityID ent_b) {
    return m_manifolds.isManifoldExist(ent_a.id, ent_b.id);
}

bool PhysicsSystem::isEntityColliding(EntityID ent_id) {
    return m_collisions_count[ent_id.id] > 0;
}

CollisionManifoldStore* PhysicsSystem::getCollisionPair(EntityID ent_a, EntityID ent_b) {
    if ( !isPairColliding(ent_a, ent_b) )
        return nullptr;

    return m_manifolds.getManifold(ent_a.id, ent_b.id);
}


void PhysicsSystem::setCollisionPair(
        EntityID ent_a, EntityID ent_b, CollisionManifold* manifold,
        EntityID ref_entity
        ) {
    if ( !m_manifolds.isManifoldExist(ent_a.id, ent_b.id) ) {
        m_collisions_count[ent_a.id] += 1;
        m_collisions_count[ent_b.id] += 1;
    }
    CollisionManifoldStore* _manifold_store = m_manifolds.getManifold(ent_a.id, ent_b.id);
    _manifold_store->manifold = *manifold;
    _manifold_store->reference_entity = ref_entity;
}


void PhysicsSystem::removeCollisionPair(EntityID ent_a, EntityID ent_b) {
    if ( !isPairColliding(ent_a, ent_b) ) {
        return;
    }

    m_manifolds.removeManifoldPair(ent_a.id, ent_b.id);

    if (m_collisions_count[ent_a.id] > 0)
        m_collisions_count[ent_a.id]--;
    if (m_collisions_count[ent_b.id] > 0)
        m_collisions_count[ent_b.id]--;
}

std::vector<CollisionEvent> PhysicsSystem::getEntityCollisionManifolds(EntityID ent_id) {
     std::vector<CollisionEvent> coll_events;
     if ( !isEntityColliding(ent_id) )
         return coll_events;

     for (size_t i=0; i<m_num_entities_with_collider; ++i) {
         EntityID ent_2 = m_entities_with_collider[i];
         if ( ent_id.id == ent_2.id )
             continue;

         CollisionManifoldStore* _manifold_store = getCollisionPair(ent_id, ent_2);
         if ( !_manifold_store )
             continue;

         CollisionEvent event = CollisionEvent(ent_2, _manifold_store->manifold);
         if ( _manifold_store->reference_entity.id != ent_id.id ) {
             event.manifold.flipNormal();
         }

         coll_events.push_back(event);
     }

     return coll_events;
}


Result<VoidResult, GameError> PhysicsSystem::Initialize(Game* game, EntityManager* entity_mgr) {
    for (size_t cindex=0; cindex<MAX_COMPONENTS; ++cindex) {
        if (!(this->m_physics_components_mask[cindex]))
            continue; // skip non-collider components

        std::bitset<MAX_COMPONENTS> mask;
        mask.reset();
        mask.set(cindex);
        for (auto ent_id : EntityView(entity_mgr, mask, false)) {
            auto collider = entity_mgr->getEntityComponentCasted<Collider>(
                    ent_id, cindex);
            collider->__setEntity(ent_id);
            collider->__physics_system = this;
        }
    }

    return ResultOK;
}


Result<VoidResult, GameError> PhysicsSystem::OnComponentAddedToEntity(const ComponentDataPayload* component_info, EntityID entity_id) {

    if (component_info->ctag != CTAG_Collider)
        return ResultOK;

    return ResultOK;
}


Result<VoidResult, GameError> PhysicsSystem::OnEntityDestroyed(EntityManager* entity_mgr, EntityID entity_id) {
    if ( !isEntityColliding(entity_id) )
        return ResultOK;

    for (size_t i=0; i<m_num_entities_with_collider; ++i) {
        const EntityID entity_2 = m_entities_with_collider[i];
        if ( entity_id.id == entity_2.id )
            continue;

        removeCollisionPair(entity_id, entity_2);
    }

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


void PhysicsSystem::__updateColliderObjectList(EntityManager* entity_mgr) {
    // list all colliders
    m_num_entities_with_collider = 0;
    m_collider_objects_count = 0;
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
            m_collider_objects[m_collider_objects_count] = collider_object;

            m_entities_with_collider[m_num_entities_with_collider] = ent_id;
            ++m_num_entities_with_collider;
            ++m_collider_objects_count;
        }
    }
}


void PhysicsSystem::__colliderCheckCollision(
        ColliderObject* collider,
        std::function<void (
            bool, CollisionManifold*, bool*, ColliderObject*, ColliderObject*)
        > result_handling_function,
        const size_t __collider_object_start_index
) {
    ColliderObject* coll_1 = collider;

    for (size_t j=__collider_object_start_index; j<m_collider_objects_count; ++j) {
        ColliderObject* coll_2 = &m_collider_objects[j];

        if ( coll_1 == coll_2 )
            continue;

        if ( coll_1->entity.id == coll_2->entity.id )
            continue;

        if ( coll_1->collider->is_static && coll_2->collider->is_static )
            continue;

        CollisionManifold manifold = CollisionManifold(Vector2::RIGHT, 0.0);
        manifold.point_count = 0;
        bool is_body_1_the_ref = false;
        bool is_colliding = true;
        if (!axisAlignedBoundingSquareCheck(
                    coll_1->collider->__transform.__globalPosPtr(), coll_1->collider->__bounding_radius,
                    coll_2->collider->__transform.__globalPosPtr(), coll_2->collider->__bounding_radius
                    )) {
            is_colliding = false;
            // let callback handle collision
            if (result_handling_function) {
                result_handling_function(
                        is_colliding,
                        &manifold,
                        &is_body_1_the_ref,
                        coll_1, coll_2
                        );
            }
            continue;
        }

        // pair checking
        switch (coll_1->collider_tag) {
            case COLTAG_Box:
                switch (coll_2->collider_tag) {
                    case COLTAG_Box:
                        is_colliding = boxToBoxCollision(
                                static_cast<BoxCollider*>(coll_1->collider),
                                static_cast<BoxCollider*>(coll_2->collider),
                                &manifold,
                                &is_body_1_the_ref
                                );
                        break;
                    case COLTAG_Circle:
                        is_colliding = boxToCircleCollision(
                                static_cast<BoxCollider*>(coll_1->collider),
                                static_cast<CircleCollider*>(coll_2->collider),
                                &manifold,
                                &is_body_1_the_ref
                                );
                        break;
                    case COLTAG_Polygon:
                        is_colliding = boxToPolygonCollision(
                                static_cast<BoxCollider*>(coll_1->collider),
                                static_cast<PolygonCollider*>(coll_2->collider),
                                &manifold,
                                &is_body_1_the_ref
                                );
                        break;
                    case COLTAG_Capsule:
                        is_colliding = boxToCapsuleCollision(
                                static_cast<BoxCollider*>(coll_1->collider),
                                static_cast<CapsuleCollider*>(coll_2->collider),
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
                                static_cast<CircleCollider*>(coll_1->collider),
                                &manifold,
                                &is_body_1_the_ref
                                );
                        is_body_1_the_ref = !is_body_1_the_ref;
                        break;
                    case COLTAG_Circle:
                        is_colliding = circleToCircleCollision(
                                static_cast<CircleCollider*>(coll_1->collider),
                                static_cast<CircleCollider*>(coll_2->collider),
                                &manifold,
                                &is_body_1_the_ref
                                );
                        break;
                    case COLTAG_Polygon:
                        is_colliding = circleToPolygonCollision(
                                static_cast<CircleCollider*>(coll_1->collider),
                                static_cast<PolygonCollider*>(coll_2->collider),
                                &manifold,
                                &is_body_1_the_ref
                                );
                        break;
                    case COLTAG_Capsule:
                        is_colliding = circleToCapsuleCollision(
                                static_cast<CircleCollider*>(coll_1->collider),
                                static_cast<CapsuleCollider*>(coll_2->collider),
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
                                static_cast<PolygonCollider*>(coll_1->collider),
                                &manifold,
                                &is_body_1_the_ref
                                );
                        is_body_1_the_ref = !is_body_1_the_ref;
                        break;
                    case COLTAG_Circle:
                        is_colliding = circleToPolygonCollision(
                                static_cast<CircleCollider*>(coll_2->collider),
                                static_cast<PolygonCollider*>(coll_1->collider),
                                &manifold,
                                &is_body_1_the_ref
                                );
                        is_body_1_the_ref = !is_body_1_the_ref;
                        break;
                    case COLTAG_Polygon:
                        is_colliding = polygonToPolygonCollision(
                                static_cast<PolygonCollider*>(coll_1->collider),
                                static_cast<PolygonCollider*>(coll_2->collider),
                                &manifold,
                                &is_body_1_the_ref
                                );
                        break;
                    case COLTAG_Capsule:
                        is_colliding = polygonToCapsuleCollision(
                                static_cast<PolygonCollider*>(coll_1->collider),
                                static_cast<CapsuleCollider*>(coll_2->collider),
                                &manifold,
                                &is_body_1_the_ref
                                );
                        break;
                }
                break;
            case COLTAG_Capsule:
                switch (coll_2->collider_tag) {
                    case COLTAG_Box:
                        is_colliding = boxToCapsuleCollision(
                                static_cast<BoxCollider*>(coll_2->collider),
                                static_cast<CapsuleCollider*>(coll_1->collider),
                                &manifold,
                                &is_body_1_the_ref
                                );
                        is_body_1_the_ref = !is_body_1_the_ref;
                        break;
                    case COLTAG_Circle:
                        is_colliding = circleToCapsuleCollision(
                                static_cast<CircleCollider*>(coll_2->collider),
                                static_cast<CapsuleCollider*>(coll_1->collider),
                                &manifold,
                                &is_body_1_the_ref
                                );
                        is_body_1_the_ref = !is_body_1_the_ref;
                        break;
                    case COLTAG_Polygon:
                        is_colliding = polygonToCapsuleCollision(
                                static_cast<PolygonCollider*>(coll_2->collider),
                                static_cast<CapsuleCollider*>(coll_1->collider),
                                &manifold,
                                &is_body_1_the_ref
                                );
                        is_body_1_the_ref = !is_body_1_the_ref;
                        break;
                    case COLTAG_Capsule:
                        is_colliding = capsuleToCapsuleCollision(
                                static_cast<CapsuleCollider*>(coll_1->collider),
                                static_cast<CapsuleCollider*>(coll_2->collider),
                                &manifold,
                                &is_body_1_the_ref
                                );
                        break;
                }
                break;
        }

        // let callback handle collision
        if (result_handling_function) {
            result_handling_function(
                    is_colliding,
                    &manifold,
                    &is_body_1_the_ref,
                    coll_1, coll_2
                    );
        }
    }
}


SDL_Renderer* renderer = nullptr;
Result<VoidResult, GameError> PhysicsSystem::FixedUpdate(double delta_time_s, EntityManager* entity_mgr) {

    this->__updateColliderObjectList(entity_mgr);

    auto result_handling_function = [this](
            bool is_colliding, CollisionManifold* manifold, bool* is_body_1_ref,
            ColliderObject* coll_1, ColliderObject* coll_2
            ) {
        // the returned manifold is in the perspective of the reference body
        // determined the reference body using `is_body_1_the_ref`

        // no manifold points means no collision
        if ( is_colliding ) {
            bool trigger_enter = false;
            // if originally no collision happening between this 2 object
            if ( !this->isPairColliding(coll_1->entity, coll_2->entity) ) {
                trigger_enter = true;
            }

            EntityID ref_entity = is_body_1_ref ? coll_1->entity : coll_2->entity;
            this->setCollisionPair(coll_1->entity, coll_2->entity, manifold, ref_entity);

            if ( trigger_enter ) {
                coll_1->collider->__triggerOnBodyEnter(coll_2->entity);
                coll_2->collider->__triggerOnBodyEnter(coll_1->entity);
            }
        }
        // manifold points means there's collision
        else {
            // if originally no collision happening between this 2 object
            if ( !this->isPairColliding(coll_1->entity, coll_2->entity) ) {
                return;
            }

            this->removeCollisionPair(coll_1->entity, coll_2->entity);

            coll_1->collider->__triggerOnBodyLeave(coll_2->entity);
            coll_2->collider->__triggerOnBodyLeave(coll_1->entity);
        }
    };

    for (size_t i=0; i<m_collider_objects_count; ++i) {
        ColliderObject* collider = &m_collider_objects[i];
        this->__colliderCheckCollision(
                collider, result_handling_function, i+1
                );
    }

    return ResultOK;
}


Result<VoidResult, GameError> PhysicsSystem::Draw(RenderContext* renderContext, EntityManager* entity_mgr) {
    renderer = renderContext->renderer;

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
                    const Vector2 coll_pos = box_coll->__transform.GlobalPosition();
                    if ( isEntityColliding(ent_id) ) {
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
                        const Vector2 vert = sceneToScreenSpace(renderContext,
                                box_coll->__polygon.getVertex(
                                    i,
                                    coll_pos,
                                    box_coll->__transform.rotation
                                    )
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
                    std::vector<CollisionEvent> coll_events = getEntityCollisionManifolds(ent_id);
                    for (auto & coll_event : coll_events) {
                        CollisionManifold manifold = coll_event.manifold;
                        for (int i=0; i<manifold.point_count; ++i) {
                            Vector2 contact = sceneToScreenSpace(
                                    renderContext, manifold.points[i]
                                    );

                            SDL_SetRenderDrawColorFloat(
                                    renderContext->renderer,
                                    0.0, 0.0, 1.0, 1.0
                                    );
                            phydebDrawCross(
                                    renderContext->renderer,
                                    &contact
                                    );
                        }
                    }

                    break;
                    }
                case COLTAG_Circle:
                    {
                    CircleCollider* circ_coll = static_cast<CircleCollider*>(coll);
                    Vector2 circ_pos = circ_coll->__transform.GlobalPosition();
                    if ( isEntityColliding(ent_id) ) {
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
                    const size_t point_counts = 16;
                    SDL_FPoint circle_points[point_counts+1];
                    for (int i=0; i<point_counts+1; i++) {
                        const float xp = circ_coll->radius*std::cos(i*2.0*M_PI/point_counts);
                        const float yp = circ_coll->radius*std::sin(i*2.0*M_PI/point_counts);
                        const Vector2 circle_point = sceneToScreenSpace(renderContext,
                                Vector2(xp + circ_pos.x, yp + circ_pos.y)
                                );
                        circle_points[i] = { circle_point.x, circle_point.y };
                     }
                    
                    const Vector2 _center = sceneToScreenSpace(renderContext, circ_pos);
                    SDL_RenderPoint(renderContext->renderer, _center.x, _center.y);
                    SDL_RenderLines(
                            renderContext->renderer, circle_points, point_counts+1
                            );

                    // manifold
                    std::vector<CollisionEvent> coll_events = getEntityCollisionManifolds(ent_id);
                    for (auto& coll_event : coll_events) {
                        CollisionManifold manifold = coll_event.manifold;
                        for (int i=0; i<manifold.point_count; ++i) {
                            Vector2 contact = sceneToScreenSpace(renderContext, manifold.points[i]);

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
                                    contact.x + manifold.normal.x*manifold.penetration_depth,
                                    contact.y + manifold.normal.y*manifold.penetration_depth
                                    );
                        }
                    }
                    
                    break;
                    }
                case COLTAG_Capsule:
                    {
                    CapsuleCollider* caps_coll = static_cast<CapsuleCollider*>(coll);
                    const Vector2 caps_pos = caps_coll->__transform.GlobalPosition();
                    const double caps_rot = caps_coll->__transform.rotation;
                    const Vector2 caps_p1 = caps_pos + Vector2::UP.rotated(caps_rot) * (caps_coll->length / 2.0);
                    const Vector2 caps_p2 = caps_pos + Vector2::DOWN.rotated(caps_rot) * (caps_coll->length / 2.0);
                    if ( isEntityColliding(ent_id) ) {
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
                    const size_t point_counts = 16;
                    SDL_FPoint circle_points_1[point_counts+1];
                    SDL_FPoint circle_points_2[point_counts+1];
                    for (int i=0; i<point_counts+1; i++) {
                        const float xp = caps_coll->radius*std::cos(i*2.0*M_PI/point_counts);
                        const float yp = caps_coll->radius*std::sin(i*2.0*M_PI/point_counts);
                        const Vector2 circle_point_1 = sceneToScreenSpace(renderContext,
                                Vector2(xp + caps_p1.x, yp + caps_p1.y)
                                );
                        const Vector2 circle_point_2 = sceneToScreenSpace(renderContext,
                                Vector2(xp + caps_p2.x, yp + caps_p2.y)
                                );
                        circle_points_1[i] = { circle_point_1.x, circle_point_1.y };
                        circle_points_2[i] = { circle_point_2.x, circle_point_2.y };
                     }

                    const Vector2 _center = sceneToScreenSpace(renderContext, caps_pos);
                    SDL_RenderPoint(renderContext->renderer, _center.x, _center.y);
                    SDL_RenderLines(
                            renderContext->renderer, circle_points_1, point_counts+1
                            );
                    SDL_RenderLines(
                            renderContext->renderer, circle_points_2, point_counts+1
                            );
                    const Vector2 right_line_offset = Vector2::RIGHT.rotated(caps_rot) * caps_coll->radius;
                    const Vector2 left_line_offset = Vector2::LEFT.rotated(caps_rot) * caps_coll->radius;
                    const Vector2 right_line_p1 = sceneToScreenSpace(renderContext, caps_p1 + right_line_offset);
                    const Vector2 right_line_p2 = sceneToScreenSpace(renderContext, caps_p2 + right_line_offset);
                    const Vector2 left_line_p1 = sceneToScreenSpace(renderContext, caps_p1 + left_line_offset);
                    const Vector2 left_line_p2 = sceneToScreenSpace(renderContext, caps_p2 + left_line_offset);
                    SDL_RenderLine(
                            renderContext->renderer,
                            right_line_p1.x, right_line_p1.y,
                            right_line_p2.x, right_line_p2.y
                            );
                    SDL_RenderLine(
                            renderContext->renderer,
                            left_line_p1.x, left_line_p1.y,
                            left_line_p2.x, left_line_p2.y
                            );

                    // manifold
                    std::vector<CollisionEvent> coll_events = getEntityCollisionManifolds(ent_id);
                    for (auto& coll_event : coll_events) {
                        CollisionManifold manifold = coll_event.manifold;
                        for (int i=0; i<manifold.point_count; ++i) {
                            Vector2 contact = sceneToScreenSpace(renderContext, manifold.points[i]);

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
                                    contact.x + manifold.normal.x*manifold.penetration_depth,
                                    contact.y + manifold.normal.y*manifold.penetration_depth
                                    );
                        }
                    }

                    break;
                    }
                case COLTAG_Polygon:
                    {
                    PolygonCollider* poly_coll = static_cast<PolygonCollider*>(coll);
                    const Vector2 coll_pos = poly_coll->__transform.GlobalPosition();
                    if ( isEntityColliding(ent_id) ) {
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
                        const Vector2 vert =
                            sceneToScreenSpace(renderContext,
                                    poly_coll->__polygon.getVertex(
                                        i,
                                        coll_pos,
                                        poly_coll->__transform.rotation
                                        )
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
                    std::vector<CollisionEvent> coll_events = getEntityCollisionManifolds(ent_id);
                    for (auto& coll_event : coll_events) {
                        CollisionManifold manifold = coll_event.manifold;
                        for (int i=0; i<manifold.point_count; ++i) {
                            Vector2 contact = sceneToScreenSpace(renderContext, manifold.points[i]);

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
                                    contact.x + manifold.normal.x*manifold.penetration_depth,
                                    contact.y + manifold.normal.y*manifold.penetration_depth
                                    );
                        }
                    }

                    break;
                    }

            }

#ifdef PHYSICS_DEBUG_DRAW_SHOW_BOUNDING_BOX
            SDL_FRect bbox_rect = {
                coll->__transform.__globalPosPtr()->x - coll->__bounding_radius,
                coll->__transform.__globalPosPtr()->y - coll->__bounding_radius,
                coll->__bounding_radius*2,
                coll->__bounding_radius*2
            };
            SDL_SetRenderDrawColorFloat(
                    renderContext->renderer,
                    0.0, 1.0, 0.0, 1.0
                    );
            SDL_RenderRect(
                    renderContext->renderer,
                    &bbox_rect
                    );
#endif

#ifdef PHYSICS_DEBUG_DRAW_SHOW_ENTITY_ID
            SDL_SetRenderDrawColorFloat(
                    renderContext->renderer,
                    0.0, 1.0, 0.0, 1.0
                    );
            SDL_RenderDebugText(
                    renderContext->renderer,
                    coll->__transform.GlobalPosition().x, coll->__transform.GlobalPosition().y,
                    std::string("ent id: ").append(std::to_string(coll->entity().id)).c_str()
                    );
#endif

        }
    }

    return ResultOK;
#endif

    return ResultOK;
}
