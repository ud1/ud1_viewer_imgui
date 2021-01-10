cmake_minimum_required(VERSION 3.13)
project(ud1_viewer_imgui)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE imgui_Sources
        imgui/*.cpp imgui/*.h nanovg/nanovg.c)

SET(CMAKE_CXX_FLAGS "-D_LINUX -O0 -g -Wall -Wno-unknown-pragmas -Werror=return-type ")

include_directories(imgui include nanovg)

add_executable(ud1_viewer_imgui main.cpp ${imgui_Sources} include/GL/gl3w.c ObjArray.cpp ObjArray.hpp Frame.cpp Frame.h format.hpp format.cpp StateHolder.cpp StateHolder.hpp SceneRenderer.cpp SceneRenderer.hpp Settings.cpp Settings.hpp TcpServer.cpp TcpServer.hpp Shader.cpp Shader.hpp Camera.cpp Camera.hpp)

find_package(glfw3)

target_link_libraries(ud1_viewer_imgui glfw3 dl GL zmqpp zmq pthread X11 GLEW)
