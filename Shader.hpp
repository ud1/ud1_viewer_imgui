#ifndef UD1_VIEWER_IMGUI_SHADER_HPP
#define UD1_VIEWER_IMGUI_SHADER_HPP

#include <map>
#include <string>
#include <GL/glew.h>

struct Shader
{
    Shader(const std::string &name) : shaderName(name) {}
    GLuint program = 0;
    std::map<std::string, GLint> uniforms;
    std::string shaderName;

    bool buildShaderProgram(const char *vsText, const char *fsText);
    bool buildShaderProgram(const char *vsText, const char *gsText, const char *fsText);
};


#endif //UD1_VIEWER_IMGUI_SHADER_HPP
