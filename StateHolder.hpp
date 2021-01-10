#ifndef UD1_VIEWER_IMGUI_STATEHOLDER_HPP
#define UD1_VIEWER_IMGUI_STATEHOLDER_HPP

#include "Frame.h"

enum FrameTransitionType
{
    FRAME_TR_STOP,
    FRAME_TR_RUN,
    FRAME_TR_BEFORE_LAST,
    FRAME_TR_LAST,
};

class StateHolder {
    std::vector<std::shared_ptr<Frame>> frames;
    std::shared_ptr<StaticObjects> currentStaticObjects = std::make_shared<StaticObjects>();
    int currentFrame;

    mutable std::recursive_mutex mutex;
    FrameTransitionType frameTransitionType = FRAME_TR_BEFORE_LAST;

public:
    void renderTicksScrollbar();
    void process(const Obj &obj);
    void onNewConnection();
    void tick();
    void loadFromFile(const std::string &file);

    Frame *getFrame()
    {
        if (currentFrame >= 0 && currentFrame < frames.size())
            return frames[currentFrame].get();

        return nullptr;
    }

    const StaticObjects* getCurrentStaticObjects()
    {
        Frame *frame = getFrame();
        if (frame != nullptr)
            return frame->staticObjects.get();

        return currentStaticObjects.get();
    }
};


#endif //UD1_VIEWER_IMGUI_STATEHOLDER_HPP
