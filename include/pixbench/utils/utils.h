#ifndef UTILS_HEADER
#define UTILS_HEADER


#include <SDL3/SDL_stdinc.h>
#include <sys/types.h>

void PrepareRandomGenerator();

u_int32_t GenerateRandomUInt32();

class Color {
private:
public:
    Uint8 r, g, b, a;
    Color (Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
        this->r = r;
        this->g = g;
        this->b = b;
        this->a = a;
    }

    Color() : Color(0, 0, 0, 255) {}

    static Color GetWhite() {
        return Color(255, 255, 255, 255);
    }

    static Color GetBlack() {
        return Color();
    }

    static Color GetGray() {
        return Color(170, 170, 170, 255);
    }
};

#endif
