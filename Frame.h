//
// Created by denis on 14.11.2020.
//

#ifndef UD1_VIEWER_IMGUI_FRAME_H
#define UD1_VIEWER_IMGUI_FRAME_H

#include <vector>
#include <mutex>
#include <set>
#include "format.hpp"

class StaticObjects
{
public:
    int w = 1000, h = 1000;
    bool mode3d = false;

    P minP, maxP;
    double hMin, hMax, cellSize;

    std::vector<SObj> objs;
};

class Frame {
    void filterLogs();
public:
    std::shared_ptr<StaticObjects> staticObjects;

    unsigned tick = 0;
    std::vector<Obj> objs;
    std::string logs;
    std::string filteredLogs;
    std::string filter;
    bool filtered = false;
    mutable std::recursive_mutex mutex;

    Frame(const std::shared_ptr<StaticObjects> &staticObjects);

    void appendLog(const std::string &log);
    void renderTree(std::set<std::string> &expanded);
    void renderLogs(const std::string &filter);
};


#endif //UD1_VIEWER_IMGUI_FRAME_H
