#ifndef UD1_VIEWER_IMGUI_CAMERA_HPP
#define UD1_VIEWER_IMGUI_CAMERA_HPP

#include "myutils3d.hpp"
#include "Settings.hpp"

class Camera
{
public:
    Camera(Settings &settings);
    void init();
    Q orientation() const;
    M4 getMatrix() const;
    M4 getVP() const;
    M4 getProj() const;
    void rotate(float x, float y);
    void move(float f, float r, float u);
    V3 transform(float f, float r, float u) const;
    V3 untransform(const V3 &v) const;

    float aspectRatio, fovMultiplier;
    float zNear, zFar;
    V3 position;
    float yaw, pitch;
    float sens;

    Settings &settings;
    float getFOV() const
    {
        return settings.fov / 180.0f * M_PI;
    }
};


#endif //UD1_VIEWER_IMGUI_CAMERA_HPP
