#include "pch.h"
#include "Camera.h"

Camera::Camera()
{
}

glm::mat4 Camera::GetView()
{
    return glm::lookAt(GetPos(), GetPos() + GetDir(), GetUp());
}

glm::mat4 Camera::GetProj(const uint32_t& width, const uint32_t& height)
{
    return glm::perspective(glm::radians(45.0f),  (float)width/ (float)height, _nearZ, _farZ);
}

void Camera::moveForward(float dt)
{
    _position += dt * _viewDir * _speed;
}

void Camera::moveRight(float dt)
{
    _position += -dt * _rightDir * _speed;
}

void Camera::rotate(double mouse_dx, double mouse_dy)
{
    if (mouse_dx == 0 && mouse_dy == 0) return;
    mouse_dx = -mouse_dx;

    glm::mat4 rotateMatrixX = glm::rotate(glm::mat4(1.0f), glm::radians((float)mouse_dx * _rotateSpeed), _upDir);
    
    glm::vec4 viewDir = rotateMatrixX * glm::vec4(_viewDir,1.0f);

    _viewDir = glm::vec3(viewDir);
    _rightDir = glm::cross(_upDir, _viewDir);
    
    
    glm::mat4 rotateMatrixY = glm::rotate(glm::mat4(1.0f), glm::radians((float)mouse_dy * _rotateSpeed), _rightDir);

    glm::vec4 upDir = rotateMatrixY * glm::vec4(_upDir, 1.0f);
    _upDir = glm::vec3(upDir);
    _viewDir = glm::cross(_rightDir, _upDir);

}

void Camera::Update(float dt, bool* keyPressed,double mouse_dx,double mouse_dy)
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
    rotate(mouse_dx, mouse_dy);
}
