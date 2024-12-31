#include "pch.h"
#include "Camera.h"

Camera::Camera()
{
}

glm::mat4 Camera::GetView()
{
    return glm::mat4();
}

glm::mat4 Camera::GetProj()
{
    return glm::mat4();
}

void Camera::moveForward(float dt)
{
    _position += dt * _viewDir * _speed;
}

void Camera::moveRight(float dt)
{
    _position += dt * _rightDir * _speed;
}

void Camera::Update(float dt, bool* keyPressed)
{
    if (keyPressed[GLFW_KEY_W]) {
        moveForward(dt);
    }
    if (keyPressed[GLFW_KEY_D]) {
        moveRight(dt);
    }
    if (keyPressed[GLFW_KEY_S]) {
        moveForward(-dt);
    }
    if (keyPressed[GLFW_KEY_A]) {
        moveRight(-dt);
    }
}
