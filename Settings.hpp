#ifndef UD1_VIEWER_IMGUI_SETTINGS_HPP
#define UD1_VIEWER_IMGUI_SETTINGS_HPP

#include <string>
#include <vector>
#include <functional>
#include "myutils3d.hpp"

class TcpServer;

class Settings {
public:
    Settings()
    {
        load();
    }

    int port = 8400;
    bool saveToFile = false;
    bool yIsUp = true;
    std::string outputDir = "";
    float fov = 90;
    int lineThickness = 1;
    int cameraFw = 2;
    int cameraR = 0;

    int guiScale = 2;
    V3 cameraPosition;
    float cameraYaw = 0, cameraPitch = 0;

    void load();
    void save();
    void renderDialog(bool *p_open);

    V3 getCameraFw() const;
    V3 getCameraR() const;

    TcpServer *tcpServer = nullptr;
};


#endif //UD1_VIEWER_IMGUI_SETTINGS_HPP
