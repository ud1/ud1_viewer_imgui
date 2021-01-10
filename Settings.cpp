#include "Settings.hpp"
#include "imgui.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "TcpServer.hpp"

namespace pt = boost::property_tree;

V3 AXES_VECTORS[] = {
        V3(1, 0, 0),
        V3(-1, 0, 0),
        V3(0, 1, 0),
        V3(0, -1, 0),
        V3(0, 0, 1),
        V3(0, 0, -1)
};

void renderVectorChoice(const char *label, int *v)
{
    ImGui::PushID(label);

    ImGui::Text("%s", label); ImGui::SameLine();
    ImGui::RadioButton("+X", v, 0); ImGui::SameLine();
    ImGui::RadioButton("-X", v, 1); ImGui::SameLine();
    ImGui::RadioButton("+Y", v, 2); ImGui::SameLine();
    ImGui::RadioButton("-Y", v, 3); ImGui::SameLine();
    ImGui::RadioButton("+Z", v, 4); ImGui::SameLine();
    ImGui::RadioButton("-Z", v, 5);

    ImGui::PopID();
}

void Settings::renderDialog(bool *p_open) {
    ImGui::Begin("Settings", p_open);
    ImGui::InputInt("Port", &port);
    ImGui::SliderInt("Line thickness", &lineThickness, 1, 10);
    ImGui::SliderInt("GUI scale", &guiScale, 1, 2);
    ImGui::Checkbox("Save to file", &saveToFile);
    ImGui::Checkbox("Y is up", &yIsUp);

    ImGui::SliderFloat("FOV", &fov, 60.0f, 120.0f);

    renderVectorChoice("Forward vector", &cameraFw);
    renderVectorChoice("Right vector", &cameraR);

    if (ImGui::Button("Save"))
    {
        *p_open = false;
        save();
    }

    ImGui::End();
}

void Settings::load() {
    pt::ptree tree;

    try {
        pt::read_json(".ud1_viewer.json", tree);

        port = tree.get("port", 8400);
        saveToFile = tree.get("saveToFile", false);
        outputDir = tree.get("outputDir", "");
        fov = tree.get("fov", 90.0);
        lineThickness = tree.get("lineThickness", 1);
        yIsUp = tree.get("yIsUp", false);
        cameraFw = tree.get("cameraFw", 2);
        cameraR = tree.get("cameraR", 0);
        guiScale = tree.get("guiScale", 2);
        cameraPosition = V3(tree.get("camera.x", 0.0), tree.get("camera.y", 0.0), tree.get("camera.z", 0.0));
        cameraYaw = tree.get("camera.yaw", 0.0);
        cameraPitch = tree.get("camera.pitch", 0.0);
    }
    catch (pt::json_parser_error error)
    {
    }
}

void Settings::save() {
    pt::ptree tree;
    tree.put("port", port);
    tree.put("saveToFile", saveToFile);
    tree.put("outputDir", outputDir);
    tree.put("fov", fov);
    tree.put("lineThickness", lineThickness);
    tree.put("yIsUp", yIsUp);
    tree.put("cameraFw", cameraFw);
    tree.put("cameraR", cameraR);
    tree.put("guiScale", guiScale);

    tree.put("camera.x", cameraPosition.x);
    tree.put("camera.y", cameraPosition.y);
    tree.put("camera.z", cameraPosition.z);
    tree.put("camera.yaw", cameraYaw);
    tree.put("camera.pitch", cameraPitch);


    pt::write_json(".ud1_viewer.json", tree);

    if (tcpServer)
        tcpServer->settingsChanged();
}

V3 Settings::getCameraFw() const {
    return AXES_VECTORS[cameraFw];
}

V3 Settings::getCameraR() const {
    return AXES_VECTORS[cameraR];
}
