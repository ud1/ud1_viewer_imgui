// dear imgui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuiFileBrowser.h"
#include <stdio.h>
#include <signal.h>
#include <zmq.hpp>
#include "Frame.h"
#include "StateHolder.hpp"
#include "SceneRenderer.hpp"
#include "Settings.hpp"
#include "TcpServer.hpp"

// About OpenGL function loaders: modern OpenGL doesn't have a standard header file and requires individual function pointers to be loaded manually.
// Helper libraries are often used for this purpose! Here we are supporting a few common ones: gl3w, glew, glad.
// You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>    // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>    // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>  // Initialize with gladLoadGL()
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static int s_interrupted = 0;
static void s_signal_handler (int signal_value)
{
    s_interrupted = 1;
}

static void s_catch_signals ()
{
    struct sigaction action;
    action.sa_handler = s_signal_handler;
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);
    sigaction (SIGINT, &action, nullptr);
    sigaction (SIGTERM, &action, nullptr);
}

void testZmq() {
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REP);
    socket.bind ("tcp://*:5555");

    while (true) {
        zmq::message_t request;

        try {
            socket.recv (&request);
            zmq::message_t reply (0);
            socket.send (reply);
        }
        catch(zmq::error_t& e) {
        }

        if (s_interrupted)
        {
            break;
        }
    }
}

Settings settings;
StateHolder stateHolder;
TcpServer tcpServer(settings, stateHolder);
SceneRenderer sceneRenderer(settings);

imgui_addons::ImGuiFileBrowser fileDialog;

SObj sphere(const V3 &pos, float r, uint32_t color, const V3 &lightPos)
{
    SObj res;

    res["type"] = "sphere";
    res["p"] = pos;
    res["r"] = r;
    res["c"] = color;
    res["lp"] = lightPos;

    return res;
}

int main(int, char**)
{
    //testZmq();
    //return 0;


    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
    glfwWindowHint(GLFW_SAMPLES, 8);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280 * settings.guiScale, 720 * settings.guiScale, "Viewer", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    sceneRenderer.init();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsLight();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'misc/fonts/README.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    io.Fonts->AddFontFromFileTTF("../misc/fonts/Roboto-Medium.ttf", 16.0f * settings.guiScale);
    //io.Fonts->AddFontFromFileTTF("../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    bool show_demo_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    std::set<std::string> expanded;
    bool showSettings = false;

    tcpServer.start();

    bool frameIsOpen = true;

    /*{
        for (int i = 0; i < 1000; ++i)
        {
            Obj obj;

            obj.type = "tick";
            obj.props["num"] = 123u + i;
            stateHolder.process(obj);


            obj.type = "someObj";
            obj.props["i"] = 234u;

            obj.subObjs["subObj1"]["type"] = "circumference";
            obj.subObjs["subObj1"]["p"] = P(50 + i, 50 + i);
            obj.subObjs["subObj1"]["r"] = 10.0;
            obj.subObjs["subObj1"]["c"] = 0xff0000ffu;

            obj.subObjs["subObj2"]["type"] = "line";
            obj.subObjs["subObj2"]["p1"] = P(250 + i, 50);
            obj.subObjs["subObj2"]["p2"] = P(150, 50);
            obj.subObjs["subObj2"]["p3"] = P(80, 150);
            obj.subObjs["subObj2"]["c"] = 0xffff00ffu;

            obj.subObjs["subObj3"]["type"] = "poly";
            obj.subObjs["subObj3"]["p1"] = P(150, 150);
            obj.subObjs["subObj3"]["p2"] = P(150, 250);
            obj.subObjs["subObj3"]["p3"] = P(80, 230);
            obj.subObjs["subObj3"]["c"] = 0x00ff00ffu;

            stateHolder.process(obj);
        }

        {
            {
                Obj obj;
                obj.type = "staticReset";
                stateHolder.process(obj);
            }

            {
                Obj obj;
                obj.type = "tick";
                obj.props["num"] = 1u;
                stateHolder.process(obj);
            }

            {
                Obj obj;
                obj.type = "field3d";
                obj.props["minP"] = P(0, 0);
                obj.props["maxP"] = P(10 * 2, 10 * 2);
                obj.props["hMin"] = 0.0;
                obj.props["hMax"] = 10.0;
                obj.props["cellSize"] = 1.0;

                stateHolder.process(obj);
            }

            {
                double x = 10;
                double y = 10;
                double z = 10;
                double r = 1;
                V3 lightPos = V3(16, 18, 10);

                Obj obj;
                obj.type = "ball";
                obj.subObjs["sph"] = sphere(V3(x, y, z), r, 0xffff00ffu, lightPos);

                obj.subObjs["disk"]["type"] = "disk";
                obj.subObjs["disk"]["p"] = V3(x, y, 0);
                obj.subObjs["disk"]["n"] = V3(0, 0, 1.5);
                obj.subObjs["disk"]["r"] = r;
                obj.subObjs["disk"]["c"] = 0xffff00ffu;

                obj.subObjs["l"]["type"] = "line3d";
                obj.subObjs["l"]["p1"] = V3(x, y, z);
                obj.subObjs["l"]["p2"] = V3(x, y, 0);
                obj.subObjs["l"]["c"] = 0xffff00ffu;

                stateHolder.process(obj);
            }
        }
    }*/

    const size_t MAX_FILTER_SIZE = 64;
    char filter[MAX_FILTER_SIZE] = {};

    bool openDialog = false;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        {
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("Open")) {
                        openDialog = true;
                    }
                    if (ImGui::MenuItem("Exit")) {
                        break;
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("View"))
                {
                    if (ImGui::MenuItem("Frame")) {
                        frameIsOpen = true;
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Tools"))
                {
                    if (ImGui::MenuItem("Settings")) {
                        showSettings = true;
                    }
                    ImGui::EndMenu();
                }

                ImGui::EndMainMenuBar();
            }
        }

        {
            if(openDialog) {
                ImGui::OpenPopup("Open File");
                openDialog = false;
            }

            if (fileDialog.showFileDialog("Open File", imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2(700 * settings.guiScale, 310 * settings.guiScale), ".vbin"))
            {
                stateHolder.loadFromFile(fileDialog.selected_path);
            }
        }
        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        Frame *frame = stateHolder.getFrame();

        stateHolder.tick();
        {
            if (showSettings) {
                settings.renderDialog(&showSettings);
            }

            if (frameIsOpen) {
                ImGui::Begin("Frame", &frameIsOpen);
                if (frame)
                    ImGui::Text("%s", ("Frame " + std::to_string(frame->tick)).c_str());
                ImGui::Text("%s", sceneRenderer.status.c_str());
                if (!sceneRenderer.rullerStatus.empty()) {
                    ImGui::Text("%s", sceneRenderer.rullerStatus.c_str());
                }
                ImGui::BeginChild("objects", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.5f), true);

                if (frame)
                    frame->renderTree(expanded);
                ImGui::EndChild();

                ImGui::BeginChild("logs", ImVec2(0, 0), true);
                if (frame) {
                    ImGui::InputText("Filter", filter, MAX_FILTER_SIZE);
                    ImGui::SameLine();
                    if (ImGui::Button("Copy")) {
                        ImGui::SetClipboardText(frame->filteredLogs.c_str());
                    }

                    frame->renderLogs(filter);
                }
                ImGui::EndChild();

                ImGui::End();
            }

            stateHolder.renderTicksScrollbar();
        }

        sceneRenderer.imguiInputProcessing(stateHolder.getCurrentStaticObjects());

        // Rendering
        ImGui::Render();



        int display_w, display_h;
        glfwMakeContextCurrent(window);
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT);
        glClear(GL_STENCIL_BUFFER_BIT);

        sceneRenderer.setWindowSize(display_w, display_h);
        sceneRenderer.render(stateHolder.getCurrentStaticObjects(), frame);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwMakeContextCurrent(window);


        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    settings.save();

    return 0;
}
