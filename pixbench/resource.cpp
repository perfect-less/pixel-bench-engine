#include "pixbench/resource.h"


std::shared_ptr<Res_SDL_Texture> LoadSDLTexture(std::string texture_path, SDL_Renderer* renderer) {
    return std::make_shared<Res_SDL_Texture>(texture_path, renderer);
}
