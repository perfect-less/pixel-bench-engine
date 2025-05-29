#ifndef RESOURCE_HEADER
#define RESOURCE_HEADER


#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>


/*Wrapper for SDL_Texture, which can be instantiated using */
/*make shared*/
class Res_SDL_Texture {
public:
    SDL_Texture* texture;
    
    Res_SDL_Texture(std::string texture_path, SDL_Renderer* renderer, SDL_ScaleMode scale_mode = SDL_ScaleMode::SDL_SCALEMODE_NEAREST) {
        SDL_Surface* surface = SDL_LoadBMP(texture_path.c_str());
        if (surface == NULL) {
            std::cout << "Failed to load file: " << SDL_GetError() <<std::endl;
        }
        SDL_Texture* sdlTexture = SDL_CreateTextureFromSurface(renderer, surface);
        if (sdlTexture == NULL) {
            std::cout << "Failed to convert surface to texture: " << SDL_GetError() <<std::endl;
        }
        SDL_SetTextureScaleMode(sdlTexture, scale_mode);
        SDL_DestroySurface(surface);

        this->texture = sdlTexture;
    }

    ~Res_SDL_Texture() {
        SDL_DestroyTexture(texture);
    }
};


std::shared_ptr<Res_SDL_Texture> LoadSDLTexture(std::string texture_path, SDL_Renderer* renderer);


#endif
