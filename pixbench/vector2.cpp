#include "pixbench/vector2.h"
#include <cstdio>
#include <string>


Vector2::Vector2(float x, float y) {
    this->x = x;
    this->y = y;
}

Vector2 Vector2::operator* (float s) {
    return Vector2(x*s, y*s);
}

Vector2 Vector2::operator* (double s) {
    return Vector2(x*s, y*s);
}

Vector2 operator* (const float s, const Vector2& v) {
    return Vector2(v.x*s, v.y*s);
}

Vector2 operator* (const double s, const Vector2& v) {
    return Vector2(v.x*s, v.y*s);
}

Vector2 Vector2::operator+ (const Vector2& v) {
    return Vector2(x + v.x, y + v.y);
}

Vector2 Vector2::operator- (const Vector2& v) {
    return Vector2(x - v.x, y - v.y);
}

Vector2& Vector2::operator+= (const Vector2& v) {
    this->x += v.x;
    this->y += v.y;
    return *this;
}

Vector2& Vector2::operator-= (const Vector2& v) {
    this->x -= v.x;
    this->y -= v.y;
    return *this;
}


std::string Vector2::Prints() {
   return "Vector2(" + std::to_string(x) + ", " + std::to_string(y) + ")";
}
