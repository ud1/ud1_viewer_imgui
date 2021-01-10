//
// Created by denis on 15.11.2020.
//

#include "Camera.hpp"

Camera::Camera(Settings &settings) : settings(settings) {
    init();
}

void Camera::init()
{
    position = settings.cameraPosition;
    yaw = settings.cameraYaw;
    pitch = settings.cameraPitch;

    fovMultiplier = 1.0f;
    aspectRatio = 1.0f;
    zNear = 0.1f;
    zFar = 5000.0f;
    sens = 0.002f;
}

Q Camera::orientation() const
{
    V3 up = glm::cross(settings.getCameraR(), settings.getCameraFw());
    Q orientation = glm::angleAxis(0.0f, up);

    Q turn;

    turn = glm::angleAxis(pitch, settings.getCameraR());
    orientation = glm::cross(turn, orientation);

    turn = glm::angleAxis(-yaw, up);
    orientation = glm::cross(turn, orientation);

    return orientation;
}

M4 Camera::getMatrix() const
{
    Q orientation = this->orientation();
    V3 center = orientation * settings.getCameraFw() + position;
    V3 up = orientation * glm::cross(settings.getCameraR(), settings.getCameraFw());
    return glm::lookAt(position, center, up);
}

M4 Camera::getVP() const
{
    M4 proj = glm::perspective(getFOV() * fovMultiplier, aspectRatio, zNear, zFar);
    return proj * getMatrix();
}

M4 Camera::getProj() const
{
    M4 proj = glm::perspective(getFOV() * fovMultiplier, aspectRatio, zNear, zFar);
    return proj;
}

void Camera::rotate(float x, float y)
{
    yaw += x * sens;
    pitch -= y * sens;

    const float maxPitch = 0.499f * 3.1415926535f;

    if (pitch > maxPitch)
        pitch = maxPitch;

    if (pitch < -maxPitch)
        pitch = -maxPitch;

    settings.cameraYaw = yaw;
    settings.cameraPitch = pitch;
}

void Camera::move(float f, float r, float u)
{
    position += transform(f, r, u);
}

V3 Camera::transform(float f, float r, float u) const
{
    Q orientation = this->orientation();

    V3 vf, vu, vr;
    vf = orientation * this->settings.getCameraFw();
    vr = orientation * this->settings.getCameraR();
    vu = glm::cross(vr, vf);

    V3 res = V3(0.0f, 0.0f, 0.0f);
    res += vf * f;
    res += vr * r;
    res += vu * u;

    return res;
}

V3 Camera::untransform(const V3 &v) const
{
    Q orientation = glm::conjugate(this->orientation());
    V3 d = orientation * v;
    V3 res = V3(glm::dot(d, this->settings.getCameraFw()), glm::dot(d, this->settings.getCameraR()), glm::dot(d, glm::cross(this->settings.getCameraR(), this->settings.getCameraFw())));

    return res;
}

