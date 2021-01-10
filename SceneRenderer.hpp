#ifndef UD1_VIEWER_IMGUI_SCENERENDERER_HPP
#define UD1_VIEWER_IMGUI_SCENERENDERER_HPP

#include <GL/glew.h>
#include "nanovg.h"
#include "Frame.h"
#include "Settings.hpp"

struct ViewData;

class SceneRenderer {
    Settings &settings;
    NVGcontext* vg = nullptr;
    ViewData *viewData = nullptr;

    int wid = 1, heig = 1; // window size
    double zoom = 1.0;
    P zoomCenter = P(0.5, 0.5);
    P mousePos = P(0.0, 0.0);
    P rulerStart;
    bool rulerStarted = false;

    void updateZoom();

    P toLocalP(const P &p) const
    {
        P res = p / P(wid, heig);
        return toLocalRelP(res);
    }

    P toLocalRelP(const P &relP) const
    {
        P res = (zoomCenter + (relP - P(0.5, 0.5))/zoom)*P(wid, heig);
        return res;
    }

    float transformY(double y, double h)
    {
        if (settings.yIsUp)
            return h - y;

        return y;
    }

    void renderObj2d(const SObj &sobj, double h);
public:
    SceneRenderer(Settings &settings);
    ~SceneRenderer();

    void init();
    void setWindowSize(int wid, int heig);

    void imguiInputProcessing(const StaticObjects *staticObjects);

    void render(const StaticObjects *staticObjects, const Frame *frame);

    std::string status;
    std::string rullerStatus;
};


#endif //UD1_VIEWER_IMGUI_SCENERENDERER_HPP
