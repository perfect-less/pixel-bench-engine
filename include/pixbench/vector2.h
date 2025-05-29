#ifndef VECTOR_HEADER
#define VECTOR_HEADER

#include <cmath>
#include <string>
class Vector2 {
public:
    float x, y;

    Vector2(float x, float y);
    Vector2() : Vector2(0.0, 0.0) {};

    /*template<typename T>*/
    /*Vector2 operator* (T s);*/
    Vector2 operator* (float s);
    Vector2 operator* (double s);

    Vector2 operator+ (const Vector2& v);
    Vector2 operator- (const Vector2& v);
    Vector2& operator+= (const Vector2& v);
    Vector2& operator-= (const Vector2& v);

    // Return string of vector2
    std::string Prints();
}; 

Vector2 operator* (const double s, const Vector2& v);
Vector2 operator* (const float s, const Vector2& v);

#endif
