#include "pixbench/ecs.h"
#include "pixbench/vector2.h"
#include <SDL3/SDL_render.h>
#include <bitset>
#include <cmath>
#include <memory>
#include <vector>


ScriptSystem::ScriptSystem() {
    m_script_components_mask.reset();
}


void ScriptSystem::OnComponentRegistered(const ComponentDataPayload* component_info) {
    ComponentTag ctag = component_info->ctag;
    // ComponentType ctype = component_info->ctype;
    size_t cindex = component_info->cindex;

    if (ctag == CTAG_Script) {
        this->m_script_components_mask.set(cindex);
    }
}


void ScriptSystem::OnEvent (SDL_Event *event, EntityManager* entity_mgr) {
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
            script->OnEvent(event, entity_mgr, ent_id);
        }
    }
}

/*void PreDraw(RenderContext* renderContext, EntityManager* entity_mgr);*/
/*void Draw(RenderContext* renderContext, EntityManager* entity_mgr);*/
void ScriptSystem::Initialize(Game* game, EntityManager* entity_mgr) {
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
            script->Init(game, entity_mgr, ent_id);
        }
    }
};

void ScriptSystem::Update(double delta_time_s, EntityManager* entity_mgr) {
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
            script->Update(delta_time_s, entity_mgr, ent_id);
        }
    }
};


void ScriptSystem::LateUpdate(double delta_time_s, EntityManager* entity_mgr) {
    for (size_t cindex=0; cindex<MAX_COMPONENTS; ++cindex) {
        if (!(this->m_script_components_mask[cindex]))
            continue; // skip non-script components

        std::bitset<MAX_COMPONENTS> mask;
        mask.reset();
        mask.set(cindex);
        for (auto ent_id : EntityView(entity_mgr, mask, false)) {
            auto script = entity_mgr->getEntityComponentCasted<ScriptComponent>(
                    ent_id, cindex);
            script->LateUpdate(delta_time_s, entity_mgr, ent_id);
        }
    }
};


// ===================== Rendering System =====================

void RenderingSystem::OnComponentRegistered(const ComponentDataPayload* component_info) {
    ComponentTag ctag = component_info->ctag;
    size_t cindex = component_info->cindex;

    if (ctag == CTAG_Renderable) {
        this->m_renderable_components_mask.set(cindex);
    }
}


void RenderingSystem::Initialize(Game* game, EntityManager* entity_mgr) {
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
}


void RenderingSystem::LateUpdate(double delta_time_s, EntityManager* entity_mgr) {
    // Performing animation update (moving the srect)
    std::cout << "RenderingSystem::Update" << std::endl;
    
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
                
                if (auto sprite_anim = sprite->GetSpriteAnimation().lock()) {
                    sprite->current_anim_time_s += delta_time_s;
                    int elapsed_frames = std::floor(sprite->current_anim_time_s * (double)sprite_anim->animation_speed);
                    double time_residue = std::fmod(sprite->current_anim_time_s, 1.0/(double)sprite_anim->animation_speed);
                    sprite->current_anim_time_s = time_residue;

                    sprite->current_frame += elapsed_frames;
                    sprite->current_frame = std::fmod(
                            sprite->current_frame,
                            1 + sprite_anim->end_sheet_index - sprite_anim->start_sheet_index
                            );

                    sprite->srect = sprite_anim->res_sheet->GetRectByFrameIndex(sprite->current_frame);
                }
                else {
                    std::cout << "ENT" << ent_id.id << " DOESN'T HAVE SPRITE ANIM" << std::endl;
                }
                // TO DO: Update animation for TileSet, etc
                // else if ...
            }
        }
    }

    std::cout << "RenderingSystem::Update::END" << std::endl;
};


void RenderingSystem::PreDraw(RenderContext* renderContext, EntityManager* entity_mgr) {
    std::cout << "RenderingSystem::PreDraw" << std::endl;
    // TO DO: visibility checks
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
                sprite->drect.x = (
                        sprite->offset.x + sprite->transform->GlobalPosition().x
                        - renderContext->camera_position.x
                        );
                sprite->drect.y = (
                        sprite->offset.y + sprite->transform->GlobalPosition().y 
                        - renderContext->camera_position.y
                        );
                
                SDL_FRect r = sprite->drect;
                float screen_w = renderContext->camera_size.x;
                float screen_h = renderContext->camera_size.y;
                if (
                        !(
                            std::max(r.x+r.w, screen_w) - std::min(r.x, 0.0f) < (screen_w+r.w)
                            &&
                            std::max(r.y+r.h, screen_h) - std::min(r.y, 0.0f) < (screen_h+r.h)
                         )
                   ) { // skip if not visible
                    continue;
                }
            }
            else if (renderable->getRenderableTag() == RCTAG_Tile) {
                // TODO: check if tile rect is visible
                Tile* tile = static_cast<Tile*>(renderable);
                if (!tile) 
                    continue;

                if (auto tile_map = tile->getTileMap().lock()) {
                    Vector2 tile_pos = tile->transform->GlobalPosition();
                    tile->drect.x = tile_pos.x - renderContext->camera_position.x;
                    tile->drect.y = tile_pos.y - renderContext->camera_position.y;
                    tile->drect.w = tile_map->width;
                    tile->drect.h = tile_map->height;
                    SDL_FRect r = tile->drect;

                    float screen_w = renderContext->camera_size.x;
                    float screen_h = renderContext->camera_size.y;

                    if (
                            !(
                                std::max(r.x+r.w, screen_w) - std::min(r.x, 0.0f) < (screen_w+r.w)
                                &&
                                std::max(r.y+r.h, screen_h) - std::min(r.y, 0.0f) < (screen_h+r.h)
                             )
                       ) { // skip if not visible
                        continue;
                    }
                    
                }
            }
            else if (renderable->getRenderableTag() == RCTAG_Script) {
                // TODO: check if drawable script rect inside the screen
                CustomRenderable* custom_renderable = static_cast<CustomRenderable*>(renderable);
                custom_renderable->drect.x = (
                        custom_renderable->offset.x + custom_renderable->transform->GlobalPosition().x
                        - renderContext->camera_position.x
                        );
                custom_renderable->drect.y = (
                        custom_renderable->offset.y + custom_renderable->transform->GlobalPosition().y 
                        - renderContext->camera_position.y
                        );
                
                SDL_FRect r = custom_renderable->drect;
                float screen_w = renderContext->camera_size.x;
                float screen_h = renderContext->camera_size.y;
                if (
                        !(
                            std::max(r.x+r.w, screen_w) - std::min(r.x, 0.0f) < (screen_w+r.w)
                            &&
                            std::max(r.y+r.h, screen_h) - std::min(r.y, 0.0f) < (screen_h+r.h)
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
}


void RenderingSystem::Draw(RenderContext* renderContext, EntityManager* entity_mgr) {
    std::cout << "RenderingSystem::Draw" << std::endl;
    
    for (RenderableComponent* renderable : ordered_renderables) {
        if (renderable->getRenderableTag() == RCTAG_Sprite) {
            Sprite* sprite = static_cast<Sprite*>(renderable);
            bool err = SDL_RenderTextureRotated(
                    renderContext->renderer,
                    sprite->texture->texture,
                    &(sprite->srect), &(sprite->drect),
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
        // TODO: Implement for other renderable types
        // else if (renderable->getRenderableTag() == RCTAG_Tile) {...
        else if (renderable->getRenderableTag() == RCTAG_Script) {
            CustomRenderable* custom_renderable = static_cast<CustomRenderable*>(renderable);
            custom_renderable->Draw(renderContext, entity_mgr);
        }
        else if (renderable->getRenderableTag() == RCTAG_Tile) {
            // Notes: let's skip tiles animation for now
            // Find the ranges of tiles in the map to render

            Tile* tile = static_cast<Tile*>(renderable);

            auto tile_map = tile->getTileMap().lock();
            if (!tile_map)
                continue;

            auto atlass = tile_map->getAtlass().lock();
            if (!atlass)
                continue;
            
            SDL_FRect r = tile->drect;
            auto cam_size = renderContext->camera_size;
            int start_col = std::floor(std::max(0.0f, -r.x) / tile_map->tile_w);
            int start_row = std::floor(std::max(0.0f, -r.y) / tile_map->tile_h);
            int end_col = tile_map->columns - std::floor(
                    std::max(0.0f, (r.x+tile_map->width -  cam_size.x)) /
                    tile_map->tile_w
                    );
            int end_row = tile_map->rows - std::floor(
                    std::max(0.0f, (r.y+tile_map->height -  cam_size.y)) /
                    tile_map->tile_h
                    );

            // Perform for loop to draw tiles
            SDL_FRect srect, drect;
            drect.w = tile_map->tile_w;
            drect.h = tile_map->tile_h;
            for (int c=start_col; c<end_col; ++c) {
                for (int r=start_row; r<end_row; ++r) {

                    int tile_id = tile_map->getTileIDbyTilePosition(r, c);
                    if (tile_id == 0)
                        continue;

                    srect = atlass->getRectByIndex(tile_id-1);
                    drect.x = tile->drect.x + c * tile_map->tile_w;
                    drect.y = tile->drect.y + r * tile_map->tile_h;

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
}
