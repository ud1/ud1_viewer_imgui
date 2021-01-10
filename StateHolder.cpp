#include "imgui.h"
#include "StateHolder.hpp"
#include <fstream>
#include <boost/endian/conversion.hpp>

void StateHolder::renderTicksScrollbar() {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

    float height = ImGui::GetTextLineHeightWithSpacing() + 1;


    ImVec2 window_pos = ImVec2(0, io.DisplaySize.y - height);
    ImVec2 window_pos_pivot = ImVec2(0, 0);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, height));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    if (ImGui::Begin("Ticks", nullptr, window_flags))
    {
        bool isPressed = false;
        switch (frameTransitionType)
        {
            case FRAME_TR_STOP:
                isPressed = ImGui::Button("STP");
                break;
            case FRAME_TR_RUN:
                isPressed = ImGui::Button("RUN");
                break;
            case FRAME_TR_BEFORE_LAST:
                isPressed = ImGui::Button("BLS");
                break;
            case FRAME_TR_LAST:
                isPressed = ImGui::Button("LST");
                break;
        }
        if (isPressed)
        {
            frameTransitionType = (FrameTransitionType) ((frameTransitionType + 1) % 4);
        }
        ImGui::SameLine();
        ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());
        ImGui::SliderInt("", &currentFrame, 0, frames.empty() ? 0 : ((int)frames.size() - 1));

        if (ImGui::IsItemHovered())
        {
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel < 0 && currentFrame > 0)
                --currentFrame;
            else if (wheel > 0 && currentFrame + 1 < frames.size())
                ++currentFrame;
        }
    }

    ImGui::End();

    ImGui::PopStyleVar(3);
}

void StateHolder::process(const Obj &obj) {
    if (obj.type == "tick")
    {
        frames.push_back(std::make_shared<Frame>(currentStaticObjects));
        Frame &frame = **frames.rbegin();
        frame.tick = obj.getIntProp("num");

        if (frameTransitionType == FRAME_TR_LAST)
            currentFrame = frames.size();
        else if (frameTransitionType == FRAME_TR_BEFORE_LAST)
            currentFrame = ((int) frames.size()) - 1;

        return;
    }

    if (obj.type == "fieldSize")
    {
        std::lock_guard<std::recursive_mutex> guard(mutex);
        currentStaticObjects->w = obj.getIntProp("w");
        currentStaticObjects->h = obj.getIntProp("h");
        currentStaticObjects->mode3d = false;

        return;
    }

    if (obj.type == "field3d")
    {
        std::lock_guard<std::recursive_mutex> guard(mutex);
        currentStaticObjects->mode3d = true;

        currentStaticObjects->minP = obj.getPProp("minP");
        currentStaticObjects->maxP = obj.getPProp("maxP");
        currentStaticObjects->hMin = obj.getDoubleProp("hMin");
        currentStaticObjects->hMax = obj.getDoubleProp("hMax");
        currentStaticObjects->cellSize = obj.getDoubleProp("cellSize");

        return;
    }

    if (obj.type == "static")
    {
        for (const auto &p : obj.subObjs)
            currentStaticObjects->objs.push_back(p.second);

        return;
    }

    if (obj.type == "staticReset")
    {
        std::lock_guard<std::recursive_mutex> guard(mutex);
        StaticObjects *old = &*currentStaticObjects;
        currentStaticObjects = std::make_shared<StaticObjects>();
        currentStaticObjects->w = old->w;
        currentStaticObjects->h = old->h;

        return;
    }

    if (frames.empty())
        return;

    Frame &frame = **frames.rbegin();
    {
        std::lock_guard<std::recursive_mutex> guard(frame.mutex);

        if (obj.type == "log")
        {
            std::string str = getStr("text", obj.props);
            frame.appendLog(str);
        }
        else
        {
            frame.objs.push_back(obj);
        }
    }
}

void StateHolder::tick() {
    if (frameTransitionType == FRAME_TR_RUN && currentFrame + 1 < frames.size()) {
        currentFrame++;
    }
}

void StateHolder::onNewConnection() {
    frames.clear();
    currentFrame = 0;
    currentStaticObjects = std::make_shared<StaticObjects>();
}

void StateHolder::loadFromFile(const std::string &fileName) {
    std::ifstream file;
    file.open(fileName, std::ofstream::binary);
    if (file)
    {
        onNewConnection();
        uint32_t size;

        file.read(reinterpret_cast<char *>(&size), sizeof(size));
        if (!file)
            return;

        size = boost::endian::big_to_native(size);

        std::string data;
        data.resize(size);

        file.read(data.data(), size);

        if (!file)
            return;

        std::istringstream iss(data);

        Obj obj = readObj(iss);
        if (iss)
        {
            process(obj);
        }
    }
}
