#ifndef GAME_CONFIG_HEADER
#define GAME_CONFIG_HEADER

#include "pixbench/utils/utils.h"
#include <SDL3/SDL_render.h>
#include <string>


class GameConfig {
public:
    int window_width  = 640;
    int window_height = 480;

    bool enable_joystick_and_gamepad = true;

    bool render_vsync_enabled = true;
    Color render_clear_color = Color::GetBlack();

    SDL_RendererLogicalPresentation render_logical_presentation = SDL_RendererLogicalPresentation::SDL_LOGICAL_PRESENTATION_DISABLED;

    std::string game_title = "Untitled Game";
    std::string game_version = "1.0.0";
    std::string game_identifier = "com.example.pixelnode-game";
};

#endif
