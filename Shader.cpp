#include "Shader.hpp"
#include <vector>
#include <iostream>

static GLuint createShader(GLenum eShaderType, const char *dataStr, const std::string &shaderName)
{
    GLuint shader = glCreateShader(eShaderType);
    glShaderSource(shader, 1, &dataStr, NULL);

    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE)
    {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

        GLchar strInfoLog[4096];
        glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);

        char strShaderType[16];

        switch (eShaderType)
        {
            case GL_VERTEX_SHADER:
                sprintf(strShaderType, "vertex");
                break;

            case GL_GEOMETRY_SHADER:
                sprintf(strShaderType, "geometry");
                break;

            case GL_FRAGMENT_SHADER:
                sprintf(strShaderType, "fragment");
                break;
        }

        std::cerr << "Compile failure in " <<  strShaderType << " shader(" << shaderName.c_str() << "):\n" << strInfoLog;
        return -1;
    }
    else
    {
        std::cerr << "Shader compiled sucessfully! " << shaderName.c_str();
    }

    return shader;
}

bool Shader::buildShaderProgram(const char *vsText, const char *fsText)
{
    GLuint vertexShader;
    GLuint fragmentShader;

    vertexShader = createShader(GL_VERTEX_SHADER, vsText, shaderName);
    fragmentShader = createShader(GL_FRAGMENT_SHADER, fsText, shaderName);

    program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program); //linking!

    //error checking
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if (status == GL_FALSE)
    {
        GLint infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

        GLchar strInfoLog[4096];
        glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
        std::cerr << "Shader linker failure " << shaderName.c_str() << ": " << strInfoLog;
        return false;
    }
    else
    {
        std::cerr << "Shader linked sucessfully! " << shaderName.c_str();
    }

    /*glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);*/

    GLint nuni;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &nuni);
    char name[256];

    for (GLint i = 0; i < nuni; ++i)
    {
        GLint size;
        GLenum type;

        glGetActiveUniform(program, i, sizeof(name), NULL, &size, &type, name);
        GLint location = glGetUniformLocation(program, name);
        uniforms[name] = location;
        std::cerr << "Shader " << shaderName.c_str() << ": " << name << " " << location;
    }

    return true;
}

bool Shader::buildShaderProgram(const char *vsText, const char *gsText, const char *fsText)
{
    GLuint vertexShader;
    GLuint geometryShader;
    GLuint fragmentShader;

    vertexShader = createShader(GL_VERTEX_SHADER, vsText, shaderName);
    geometryShader = createShader(GL_GEOMETRY_SHADER, gsText, shaderName);
    fragmentShader = createShader(GL_FRAGMENT_SHADER, fsText, shaderName);

    program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, geometryShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program); //linking!

    //error checking
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if (status == GL_FALSE)
    {
        GLint infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

        GLchar strInfoLog[4096];
        glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);

        std::cerr << "Shader linker failure " << shaderName.c_str() << ": " << strInfoLog;
        return false;
    }
    else
    {
        std::cerr << "Shader linked sucessfully! " << shaderName.c_str();
    }

    /*glDetachShader(program, vertexShader);
    glDetachShader(program, geometryShader);
    glDetachShader(program, fragmentShader);*/

    GLint nuni;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &nuni);
    char name[256];

    for (GLint i = 0; i < nuni; ++i)
    {
        GLint size;
        GLenum type;

        glGetActiveUniform(program, i, sizeof(name), NULL, &size, &type, name);
        GLint location = glGetUniformLocation(program, name);
        uniforms[name] = location;
        std::cerr << "Shader "<< shaderName.c_str() << ": " << name << " " << location;
    }

    return true;
}
