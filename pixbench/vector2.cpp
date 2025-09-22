#include "pixbench/vector2.h"
#include <cmath>
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

const Vector2 Vector2::operator+ (const Vector2& v) const {
    return Vector2(x + v.x, y + v.y);
}

Vector2 Vector2::operator- (const Vector2& v) {
    return Vector2(x - v.x, y - v.y);
}

const Vector2 Vector2::operator- (const Vector2& v) const {
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


void Vector2::rotate(double theta_rad) {
    const float x = this->x*std::cos(theta_rad) - this->y*std::sin(theta_rad);
    const float y = this->x*std::sin(theta_rad) + this->y*std::cos(theta_rad);

    this->x = x;
    this->y = y;
}


Vector2 Vector2::rotated(double theta_rad) {
    const float x = this->x*std::cos(theta_rad) - this->y*std::sin(theta_rad);
    const float y = this->x*std::sin(theta_rad) + this->y*std::cos(theta_rad);
    
    return Vector2(x, y);
}

const Vector2 Vector2::rotated(double theta_rad) const {
    const float x = this->x*std::cos(theta_rad) - this->y*std::sin(theta_rad);
    const float y = this->x*std::sin(theta_rad) + this->y*std::cos(theta_rad);

    return Vector2(x, y);
}

double Vector2::sqrMagnitude() {
    return this->x*this->x + this->y*this->y;
}

const double Vector2::sqrMagnitude() const {
    return this->x*this->x + this->y*this->y;
}

double Vector2::magnitude() {
    const double sqr_mag = this->sqrMagnitude();
    return std::sqrt(sqr_mag);
}

const double Vector2::magnitude() const {
    const double sqr_mag = this->sqrMagnitude();
    return std::sqrt(sqr_mag);
}

void Vector2::normalize() {
    if ( this->sqrMagnitude() == 0.0 )
        return;

    const double mag = this->magnitude();
    this->x /= mag;
    this->y /= mag;
}

Vector2 Vector2::normalized() {
    if ( this->sqrMagnitude() == 0.0 )
        return Vector2(this->x, this->y);

    const double mag = this->magnitude();
    return Vector2(this->x/mag, this->y/mag);
}


const Vector2 Vector2::normalized() const {
    if ( this->sqrMagnitude() == 0.0 )
        return Vector2(this->x, this->y);

    const double mag = this->magnitude();
    return Vector2(this->x/mag, this->y/mag);
}


float Vector2::dotProduct(Vector2 v1, Vector2 v2) {
    return v1.x * v2.x + v1.y * v2.y;
}


double Vector2::AngleToRight(Vector2 v) {
    double theta = std::acos(v.normalized().x);
    if (v.y < 0) {
        theta = 2.0*M_PI - theta;
    }

    return theta;
}


double Vector2::AngleBetween(Vector2 v1, Vector2 v2) {
    const double theta_1 = Vector2::AngleToRight(v1);
    const double theta_2 = Vector2::AngleToRight(v2);
    
    double delta = theta_2 - theta_1;
    if (delta < -M_PI)
        delta += 2.0*M_PI;
    if (delta > M_PI)
        delta -= 2.0*M_PI;

    return delta;
}


Vector2 Vector2::UP = Vector2(0.0, -1.0);
Vector2 Vector2::DOWN = Vector2(0.0, 1.0);
Vector2 Vector2::LEFT = Vector2(-1.0, -0.0);
Vector2 Vector2::RIGHT = Vector2(1.0, 0.0);
Vector2 Vector2::ZERO = Vector2(0.0, 0.0);


std::string Vector2::Prints() {
   return "Vector2(" + std::to_string(x) + ", " + std::to_string(y) + ")";
}
