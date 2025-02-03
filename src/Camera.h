// Camera.h
#pragma once
#include <glm/glm.hpp>

class Camera {
public:
    glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 Front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);
    float MoveSpeed = 5.0f;

    float FOV = 45.0f;
    float Yaw = -90.0f;
    float Pitch = 0.0f;
    float Sensitivity = 0.1f;
    float ZoomSpeed = 2.0f;

    void ProcessMovement(glm::vec3 direction, float deltaTime) {
        Position += direction * MoveSpeed * deltaTime;
    }

    void UpdateVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, glm::vec3(0.0f, 1.0f, 0.0f)));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};