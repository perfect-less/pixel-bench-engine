#ifndef RENDERER_HEADER
#define RENDERER_HEADER

#include "pixbench/resource.h"
#include "pixbench/utils.h"
#include "pixbench/vector2.h"
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>
#include <iostream>
#include <memory>

class RenderContext {
public:
    SDL_Renderer* renderer;
    SDL_Window* window;
    Vector2 camera_position;
    Vector2 camera_size;
    Color renderClearColor;

    RenderContext(
            std::string game_title,
            Vector2 camera_position, Vector2 camera_size,
            Color render_clear_color
            )
    : renderClearColor(render_clear_color)
    {
        if (!SDL_CreateWindowAndRenderer(
                    game_title.c_str(),
                    (int)camera_size.x,
                    (int)camera_size.y,
                    SDL_WINDOW_RESIZABLE,
                    /*&new_window, &new_renderer*/
                    &(this->window), &(this->renderer)
                )) { 
            // SDL_WINDOW_SHOWN for unresizeable window
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Can't initialized SDL: %s", SDL_GetError());
            /*return 3;*/
        }
        this->SetCameraContext(camera_position, camera_size);
    }
    
    ~RenderContext() {
        SDL_DestroyRenderer(this->renderer);
        SDL_DestroyWindow(this->window);
    }

    void SetCameraContext(Vector2 camera_position, Vector2 camera_size) {
        std::cout << "SetCameraContext called." << std::endl;
        this->camera_position = camera_position;
        this->camera_size = camera_size;
    }
};


/* Sprite Sheet */
class SpriteSheet {
    float start_x, start_y;
    float sprite_size_x, sprite_size_y;
    float pad_x, pad_y;
    int horizontal_count = 0;
    int vertical_count = 0;
public:
    int sheet_count;
    std::shared_ptr<Res_SDL_Texture> res_texture;
    
    SpriteSheet(
            std::shared_ptr<Res_SDL_Texture> res_texture,
            float start_x, float start_y,
            float sprite_size_x, float sprite_size_y,
            float pad_x, float pad_y
            ) {
        this->start_x = start_x;
        this->start_y = start_y;
        this->sprite_size_x = sprite_size_x;
        this->sprite_size_y = sprite_size_y;
        this->pad_x = pad_x;
        this->pad_y = pad_y;

        this->res_texture = res_texture;

        /*Check size*/
        if ((int)(start_x + sprite_size_x) < res_texture->texture->w) {
            /*ATLASS SIZE ERROR*/
        }
        if ((int)(start_y + sprite_size_y) < res_texture->texture->h) {
            /*ATLASS SIZE ERROR*/
        }

        /* Calculate sheet_size */
        this->horizontal_count = 0;
        this->vertical_count = 0;
        int hor_residual = res_texture->texture->w - start_x;
        int ver_residual = res_texture->texture->h - start_y;
        
        while (hor_residual > 0) {
            ++(this->horizontal_count);
            hor_residual -= (sprite_size_x + pad_x);
        }
        while (ver_residual > 0) {
            ++(this->vertical_count);
            ver_residual -= (sprite_size_y + pad_y);
        }
        this->sheet_count = this->horizontal_count*this->vertical_count;
    }

    SDL_FRect GetRectByFrameIndex(int frame_index) {
        if (this->sheet_count == 0) {
            // This situations should never hapens, errors should be thrown at
            // the time of the constructor being called.
            std::cout << "[][][]GetRectByFrameIndex <<sheet_count = 0>> [][][]" << std::endl;
            return SDL_FRect {.x = 0, .y = 0, .w = 0, .h = 0};
        }
        
        frame_index %= this->sheet_count;

        int x_i = frame_index % this->horizontal_count;
        int y_i = std::floor(frame_index / this->horizontal_count);

        SDL_FRect srect;
        srect.x = this->start_x + (float)x_i * (this->sprite_size_x + this->pad_x);
        srect.y = this->start_y + (float)y_i * (this->sprite_size_y + this->pad_y);
        srect.w = sprite_size_x;
        srect.h = sprite_size_y;
        return srect;
    };

    int getRows() {
        return this->vertical_count;
    }

    int getColumns() {
        return this->horizontal_count;
    }

    ~SpriteSheet() {
    }
};


/*Sprite Animation */
class SpriteAnimation {
public:
    int start_sheet_index, end_sheet_index;
    int animation_speed;
    std::shared_ptr<SpriteSheet> res_sheet;
    
    SpriteAnimation(
            std::shared_ptr<SpriteSheet> res_sheet,
            int start_sheet_index, int end_sheet_index
            ) {
        this->start_sheet_index = start_sheet_index;
        this->end_sheet_index = end_sheet_index;
        this->res_sheet = res_sheet;

        if (start_sheet_index < 0 || end_sheet_index > res_sheet->sheet_count) {
            /*TODO: ERROR*/
        }
    }

    ~SpriteAnimation() {
    }
};


/*TileSet*/
class TileSet {
private:
    SpriteSheet m_sheet;
public:
    int atlass_w, atlass_h;                         // atlass size
    int rows, columns, tile_counts;                 // number of tiles
    int tile_w, tile_h;                             // tile size

    TileSet(
            std::shared_ptr<Res_SDL_Texture> tile_texture,
            int tile_w, int tile_h,
            int margin_x = 0, int margin_y = 0
           ) 
        :
            m_sheet(tile_texture, 0, 0, tile_w, tile_h, margin_x, margin_y)
    {
        m_sheet.GetRectByFrameIndex(0);
    }

    std::shared_ptr<Res_SDL_Texture> getTexture() {
        return m_sheet.res_texture;
    }

    int getRows() {
        return m_sheet.getRows();
    }

    int getColumns() {
        return m_sheet.getColumns();
    }

    SDL_FRect getRectByIndex(int index) {
        return m_sheet.GetRectByFrameIndex(index);
    }

    int getTileCount() {
        return m_sheet.sheet_count;
    }
};


/*TileMapLayer*/
class TileMapLayer {
private:
    std::shared_ptr<TileSet> m_atlass;
    unsigned int* m_map = nullptr;
public:
    int rows, columns;              // number of tiles in a row/column
    int width, height;              // width and height of tilemaplayer int pixels
    int tile_w, tile_h;             // tile size (pixels)
    int tile_counts;                // total number of tiles

    TileMapLayer(
            std::shared_ptr<TileSet> atlass,
            int rows, int columns,
            int tile_w, int tile_h
            )
        :
            m_atlass(atlass),
            rows(rows), columns(columns),
            tile_w(tile_w), tile_h(tile_h)
    {
        tile_counts = rows*columns;

        width = tile_w * columns;
        height = tile_h * rows;

        m_map = new unsigned int[tile_counts];
        for (size_t i=0; i<tile_counts; ++i) {
            m_map[i] = 0;
        }
    }

    ~TileMapLayer() {
        if (m_map)
            delete [] m_map;
    }

    int getIndexByTile(int r, int c) { // r and c starts at 1
        return c + r*columns;
    }

    int getTileIDbyIndex(int index) {
        return m_map[index];
    }

    unsigned int* getTilePointerbyIndex(int index) {
        return &m_map[index];
    }

    int getTileIDbyTilePosition(int r, int c) {
        return getTileIDbyIndex(getIndexByTile(r, c));
    }

    unsigned int* getTilePointerbyTilePosition(int r, int c) {
        return getTilePointerbyIndex(getIndexByTile(r, c));
    }

    void clearMap(unsigned int clear_value = 0) {
        for (int i=0; i<tile_counts; ++i) {
            m_map[i] = clear_value;
        }
    }

    std::weak_ptr<TileSet> getAtlass() {
        return m_atlass;
    }
};

#endif
