#include "pixbench/ecs.h"
#include "pixbench/vector2.h"


void Transform::SetPosition(Vector2 position) {
    Vector2 parent_position = this->globalPosition - this->localPosition;
    this->globalPosition = position;
    this->localPosition = position - parent_position;
}

void Transform::SetLocalPosition(Vector2 localPosition) {
    Vector2 parent_position = this->globalPosition - this->localPosition;
    this->localPosition = localPosition;
    this->globalPosition = localPosition + parent_position;
}


