#include "Frame.h"
#include "imgui.h"
#include <sstream>

void Frame::renderTree(std::set<std::string> &expanded) {
    std::lock_guard<std::recursive_mutex> guard(mutex);

    ImGui::Columns(2, "objects", true);

    for (const Obj &obj : this->objs)
    {
        bool objExpanded = expanded.count(obj.type) > 0;
        ImGui::SetNextItemOpen(objExpanded, ImGuiCond_Always);
        if (ImGui::TreeNodeEx(obj.type.c_str(), 0, "%s", obj.type.c_str()))
        {
            if (!objExpanded)
                expanded.insert(obj.type);

            for (auto &&o : obj.subObjs)
            {
                std::string itemName = obj.type + " -> " + o.first;
                bool sobjExpanded = expanded.count(itemName) > 0;

                ImGui::SetNextItemOpen(sobjExpanded, ImGuiCond_Always);
                if (ImGui::TreeNodeEx(o.first.c_str(), 0, "%s", o.first.c_str()))
                {
                    if (!sobjExpanded)
                        expanded.insert(itemName);

                    ImGui::NextColumn();
                    ImGui::NextColumn();

                    for (const auto &p : o.second) {
                        ImGui::SetNextItemOpen(sobjExpanded, ImGuiCond_Always);
                        if (ImGui::TreeNodeEx(p.first.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet, "%s", p.first.c_str()))
                        {
                            std::string val = toString(p.second);
                            ImGui::NextColumn();
                            ImGui::Text("%s", val.c_str());
                            ImGui::NextColumn();

                            ImGui::TreePop();
                        }
                    }

                    ImGui::TreePop();
                }
                else if (sobjExpanded)
                {
                    expanded.erase(itemName);
                }
            }

            ImGui::TreePop();
        }
        else if (objExpanded)
        {
            expanded.erase(obj.type);
        }
    }

    ImGui::Columns(1);

}

void Frame::appendLog(const std::string &log) {
    if (!logs.empty())
        logs.append("\n");

    logs.append(log);
    filtered = false;
}

void Frame::renderLogs(const std::string &filter) {
    std::lock_guard<std::recursive_mutex> guard(mutex);

    if (!filtered || this->filter != filter)
    {
        this->filter = filter;
        filtered = true;
        filterLogs();
    }

    ImGui::InputTextMultiline("##logs", filteredLogs.data(), filteredLogs.size() + 1, ImVec2(-FLT_MIN, -FLT_MIN), ImGuiInputTextFlags_ReadOnly);
}

void Frame::filterLogs() {
    if (filter.empty())
    {
        filteredLogs = logs;
    }
    else
    {
        std::ostringstream res;
        std::stringstream ss(logs);
        std::string to;
        while(std::getline(ss,to,'\n'))
        {
            if (to.find(filter) != std::string::npos)
            {
                res << to << "\n";
            }
        }

        filteredLogs = res.str();
    }
}

Frame::Frame(const std::shared_ptr<StaticObjects> &staticObjects) : staticObjects(staticObjects) {}

