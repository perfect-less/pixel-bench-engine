#include "pixbench/renderer.h"


SDL_FRect sceneToCamSpace(
        RenderContext* renderContext,
        SDL_FRect rect
        ) {
    return {
        rect.x - renderContext->camera_position.x,
        rect.y - renderContext->camera_position.y,
        rect.w,
        rect.h
    };
}


Vector2 sceneToCamSpace(
        RenderContext* renderContext,
        Vector2 point
        ) {
    return Vector2(
            point.x - renderContext->camera_position.x,
            point.y - renderContext->camera_position.y
            );
}


SDL_FRect camToScreenSpace(
        RenderContext* renderContext,
        SDL_FRect rect
        ) {
    const float scale_x = renderContext->screen_size.x / renderContext->camera_size.x;
    const float scale_y = renderContext->screen_size.y / renderContext->camera_size.y;
    return {
        rect.x * scale_x,
        rect.y * scale_y,
        rect.w * scale_x,
        rect.h * scale_y,
    };
}


Vector2 camToScreenSpace(
        RenderContext* renderContext,
        Vector2 point
        ) {
    const float scale_x = renderContext->screen_size.x / renderContext->camera_size.x;
    const float scale_y = renderContext->screen_size.y / renderContext->camera_size.y;
    return Vector2(
            point.x * scale_x,
            point.y * scale_y
            );
}
