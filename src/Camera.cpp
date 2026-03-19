#include "Camera.hpp"
#include <cmath>
#include <iostream>

namespace gps {

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = glm::normalize(cameraUp);

        this->initialPosition = cameraPosition;
        this->initialTarget = cameraTarget;

        this->cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        this->cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));

        glm::vec3 direction = cameraFrontDirection;
        this->yaw = glm::degrees(std::atan2(direction.z, direction.x));
        this->pitch = glm::degrees(std::asin(direction.y));

        this->autoPresentation = false;
        this->animationTime = 0.0f;
    }

    glm::vec3 Camera::getPosition() const {
        return cameraPosition;
    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraPosition + cameraFrontDirection, cameraUpDirection);
    }

    //update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        switch (direction) {
        case MOVE_FORWARD:
            cameraPosition += speed * cameraFrontDirection;
            break;
        case MOVE_BACKWARD:
            cameraPosition -= speed * cameraFrontDirection;
            break;
        case MOVE_RIGHT:
            cameraPosition += speed * cameraRightDirection;
            break;
        case MOVE_LEFT:
            cameraPosition -= speed * cameraRightDirection;
            break;
        case MOVE_UP:
            cameraPosition += speed * cameraUpDirection;
            break;
        case MOVE_DOWN:
            cameraPosition -= speed * cameraUpDirection;
            break;
        }
    }

    //update the camera internal parameters following a camera rotate event
    //yaw - camera rotation around the y axis
    //pitch - camera rotation around the x axis
    void Camera::rotate(float pitchOffset, float yawOffset) {
        yaw += yawOffset;
        pitch += pitchOffset;

        if (pitch > 89.0f) {
            pitch = 89.0f;
        }
        if (pitch < -89.0f) {
            pitch = -89.0f;
        }

        glm::vec3 front;
        front.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
        front.y = std::sin(glm::radians(pitch));
        front.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));

        cameraFrontDirection = glm::normalize(front);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
        cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));
    }

    void Camera::scale(float factor) {
        cameraPosition *= factor;
    }

    void Camera::updateCameraTarget() {
    }

    void Camera::setYawPitch(float newYaw, float newPitch) {
        this->yaw = newYaw;
        this->pitch = newPitch;

        glm::vec3 front;
        front.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
        front.y = std::sin(glm::radians(pitch));
        front.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));

        cameraFrontDirection = glm::normalize(front);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
        cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));
    }

    float mix(float a, float b, float t) {
        return a + (b - a) * t;
    }

    void Camera::updateAutoPresentation(float deltaTime, float scale) {
        if (!autoPresentation) return;

        animationTime += deltaTime;

        float radius = 15.0f * scale;
        float height = 3.0f * scale;
        float duration = 20.0f;

        if (animationTime > duration) {
            animationTime = 0.0f;
        }

        float t = animationTime / duration;

        float angle = -90.0f + (t * 180.0f);

        cameraPosition.x = std::sin(glm::radians(angle)) * radius;
        cameraPosition.z = std::cos(glm::radians(angle)) * radius;
        cameraPosition.y = height;

        setYawPitch(-angle - 90.0f, -10.0f);
    }

    void Camera::resetToInitial() {
        cameraPosition = initialPosition;
        cameraTarget = initialTarget;

        cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);

        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, glm::vec3(0.0f, 1.0f, 0.0f)));

        cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));

        glm::vec3 direction = cameraFrontDirection;
        yaw = glm::degrees(std::atan2(direction.z, direction.x));
        pitch = glm::degrees(std::asin(direction.y));

        autoPresentation = false;
        animationTime = 0.0f;
    }
}