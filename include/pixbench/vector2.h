#ifndef VECTOR_HEADER
#define VECTOR_HEADER

#include <string>


class Vector2 {
public:
    static Vector2 UP;
    static Vector2 DOWN;
    static Vector2 LEFT;
    static Vector2 RIGHT;
    static Vector2 ZERO;

    float x, y;

    Vector2(float x, float y);
    Vector2() : Vector2(0.0, 0.0) {};

    /*template<typename T>*/
    /*Vector2 operator* (T s);*/
    Vector2 operator* (float s);
    Vector2 operator* (double s);

    Vector2 operator+ (const Vector2& v);
    const Vector2 operator+ (const Vector2& v) const;
    Vector2 operator- (const Vector2& v);
    const Vector2 operator- (const Vector2& v) const;
    Vector2& operator+= (const Vector2& v);
    Vector2& operator-= (const Vector2& v);

    double sqrMagnitude();
    const double sqrMagnitude() const;
    double magnitude();
    const double magnitude() const;

    void normalize();
    Vector2 normalized();
    const Vector2 normalized() const;

    void rotate(double theta_rad);
    Vector2 rotated(double theta_rad);
    const Vector2 rotated(double theta_rad) const;

    /*
     * Return dot product of v1 and v2
     */
    static float dotProduct(Vector2 v1, Vector2 v2);

    /*
     * Angle between Vector2::RIGHT to v1
     */
    static double AngleToRight(Vector2 v);

    /*
     * Angle between v1 to v2
     */
    static double AngleBetween(Vector2 v1, Vector2 v2);

    // Return string of vector2
    std::string Prints();
}; 

Vector2 operator* (const double s, const Vector2& v);
Vector2 operator* (const float s, const Vector2& v);


#endif
