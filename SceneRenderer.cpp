//
// Created by denis on 14.11.2020.
//

#include "SceneRenderer.hpp"

#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg.h"
#include "nanovg_gl.h"
#include "imgui.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include <sstream>

#include <GLFW/glfw3.h>

const char *RECT_VS = R"TEXT(#version 330

uniform vec3 position;

void main()
{
	gl_Position = vec4(position.xyz, 1);
}
)TEXT";

const char *RECT_FS = R"TEXT(#version 330
layout(location = 0) out vec4 outputColor;
in vec3 globalPos;
flat in vec3 norm;
uniform vec3 position;
uniform vec3 gridShift;
uniform vec3 lightPos;
uniform vec4 color;

void main()
{
    // outputColor = vec4(0.5, 0.5, 0.5, 1);

    float wid = 0.02;
    float gridScale = 0.2;

    vec3 pointPosMod = sign(mod(globalPos * gridScale + vec3(wid, wid, wid) * 0.5 + gridShift, vec3(1, 1, 1)) - vec3(wid, wid, wid));
    float val = max(0.5, min(min(pointPosMod.x, pointPosMod.y), pointPosMod.z));

    float ang = dot(norm, normalize(lightPos - globalPos));
    outputColor = vec4((max(0, ang) + 0.3)*color.xyz*val, 0.5);
}
)TEXT";


const char *RECT_GS = R"TEXT(#version 330

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform mat4 MVP;
uniform vec3 ht1;
uniform vec3 ht2;

out vec3 globalPos;
flat out vec3 norm;

void main() {
    vec3 p = gl_in[0].gl_Position.xyz;

    vec3 fposition1 = p - ht1 - ht2;
    vec3 fposition2 = p + ht1 - ht2;
    vec3 fposition3 = p - ht1 + ht2;
    vec3 fposition4 = p + ht1 + ht2;
    norm = normalize(cross(ht1, ht2));

    gl_Position = MVP * vec4(fposition1, 1);
    globalPos = fposition1;
    EmitVertex();

    gl_Position = MVP * vec4(fposition2, 1);
    globalPos = fposition2;
    EmitVertex();

    gl_Position = MVP * vec4(fposition3, 1);
    globalPos = fposition3;
    EmitVertex();

    gl_Position = MVP * vec4(fposition4, 1);
    globalPos = fposition4;
    EmitVertex();

    EndPrimitive();
}

)TEXT";

const char *GRID_VS = R"TEXT(#version 330 core

layout(location = 0) in vec3 fposition;

uniform mat4 MVP;

void main()
{
    gl_Position = MVP * vec4(fposition, 1);
}

)TEXT";

const char *GRID_FS = R"TEXT(#version 330 core

// Ouput data
layout(location = 0) out vec4 out_color;

uniform vec4 color;

void main()
{
    out_color = color;
}
)TEXT";


const char *DISK_VS = R"TEXT(#version 330

uniform vec3 position;

void main()
{
	gl_Position = vec4(position.xyz, 1);
	//gl_Position = vec4(2, 2, 0, 1);
}
)TEXT";

const char *DISK_FS = R"TEXT(#version 330

layout(location = 0) out vec4 outputColor;
in vec3 globalPos;
uniform float radius;
uniform vec3 position;
uniform vec4 color;

void main()
{
    outputColor = color;

	float rad = length(globalPos - position);
	if (rad > radius || rad < radius*0.8)
        discard;
}
)TEXT";

const char *DISK_GS = R"TEXT(#version 330

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform mat4 MVP;
uniform float radius;
uniform vec3 normal;

out vec3 globalPos;

void main() {
    vec3 p = gl_in[0].gl_Position.xyz;

    vec3 d1 = vec3(1, 0, 0);
    vec3 t1 = cross(normal, d1);
    if (dot(t1, t1) == 0)
        t1 = vec3(0, 1, 0);
    else
        t1 = normalize(t1);

    vec3 t2 = cross(normal, t1);

    t1 *= radius;
    t2 *= radius;

    vec3 fposition1 = p - t1 - t2;
    vec3 fposition2 = p + t1 - t2;
    vec3 fposition3 = p - t1 + t2;
    vec3 fposition4 = p + t1 + t2;

    gl_Position = MVP * vec4(fposition1, 1);
    globalPos = fposition1;
    EmitVertex();

    gl_Position = MVP * vec4(fposition2, 1);
    globalPos = fposition2;
    EmitVertex();

    gl_Position = MVP * vec4(fposition3, 1);
    globalPos = fposition3;
    EmitVertex();

    gl_Position = MVP * vec4(fposition4, 1);
    globalPos = fposition4;
    EmitVertex();

    EndPrimitive();
}
)TEXT";

const char *CYLINDER025_VS = R"TEXT(#version 330

uniform vec3 position;

void main()
{
	gl_Position = vec4(position.xyz, 1);
	//gl_Position = vec4(2, 2, 0, 1);
}
)TEXT";

const char *CYLINDER025_FS = R"TEXT(#version 330

layout(location = 0) out vec4 outputColor;
in vec3 globalPos;
uniform mat4 VP;
uniform mat4 M;
uniform mat4 MINV;
uniform float radius;
uniform vec3 eyePosition;
uniform vec4 color;
uniform vec3 lightPos;

void main()
{
    float sphereRadius2 = 1;
    vec3 meyePosition = (MINV * vec4(eyePosition, 1)).xyz;
    vec3 rayDirection = globalPos - meyePosition;
    vec3 rayDirectionXY = vec3(rayDirection.xy, 0);
    float rayDirectionL = length(rayDirection);
    float rayDirectionXYL = length(rayDirectionXY);

    rayDirection /= rayDirectionL;
    rayDirectionXY /= rayDirectionXYL;

    vec3 relCenter = vec3(0.5, 0.5, 0) - meyePosition;
    float dt = dot(rayDirectionXY.xy, relCenter.xy);

    vec3 cr = cross(vec3(rayDirectionXY.xy, 0), vec3(relCenter.xy, 0));
	float d2 = dot(cr, cr);
	if (d2 > sphereRadius2)
        discard;

	float coef = rayDirectionL / rayDirectionXYL;
	float delta = sqrt(sphereRadius2 - d2);
	vec3 pointPos = meyePosition + ((dt + delta) * coef) * rayDirection;
	if (pointPos.x > 0.5 || pointPos.y > 0.5 || pointPos.z > 0.5 || pointPos.z < -0.5)
        discard;

    vec3 norm = -normalize(mat3(M) * vec3(pointPos.xy - vec2(0.5, 0.5), 0));
    pointPos = (M * vec4(pointPos, 1)).xyz;

    float wid = 0.02;
    float gridScale = 0.2;

        vec3 pointPosMod = sign(mod(pointPos * gridScale + vec3(wid, wid, wid) * 0.5, vec3(1, 1, 1)) - vec3(wid, wid, wid));
	float val = max(0.5, min(min(pointPosMod.x, pointPosMod.y), pointPosMod.z));

	float ang = dot(norm, normalize(lightPos - pointPos));
	outputColor = vec4((max(0, ang) + 0.3)*color.xyz * val, 1);

	vec4 realPos = VP * vec4(pointPos, 1);

	float ndcDepth = realPos.z / realPos.w;
    gl_FragDepth = ((gl_DepthRange.diff * ndcDepth) +
        gl_DepthRange.near + gl_DepthRange.far) / 2.0;

}
)TEXT";

const char *CYLINDER025_GS = R"TEXT(#version 330

layout(points) in;
layout(triangle_strip, max_vertices = 24) out;

uniform mat4 MVP;
uniform float radius;

out vec3 globalPos;

const vec3 normals[6] = vec3[6](vec3(-0.5, 0, 0), vec3(0.5, 0, 0), vec3(0, -0.5, 0), vec3(0, 0.5, 0), vec3(0, 0, -0.5), vec3(0, 0, 0.5));
const vec3 t1s[6]     = vec3[6](vec3(0, -0.5, 0), vec3(0, 0.5, 0), vec3(0.5, 0, 0), vec3(-0.5, 0, 0), vec3(-0.5, 0, 0), vec3(0.5, 0, 0));
const vec3 t2s[6]     = vec3[6](vec3(0, 0, 0.5), vec3(0, 0, 0.5), vec3(0, 0, 0.5), vec3(0, 0, 0.5), vec3(0, 0.5, 0), vec3(0, 0.5, 0));

void main() {
    for (int i = 0; i < 6; ++i)
    {
        vec3 norm = normals[i];
        vec3 t1 = t1s[i];
        vec3 t2 = t2s[i];
        vec3 p1 = gl_in[0].gl_Position.xyz + norm;
        vec3 fposition1 = p1 - t1 - t2;
        vec3 fposition2 = p1 + t1 - t2;
        vec3 fposition3 = p1 - t1 + t2;
        vec3 fposition4 = p1 + t1 + t2;

        gl_Position = MVP * vec4(fposition1.xyz, 1);
        globalPos = fposition1;
        EmitVertex();

        gl_Position = MVP * vec4(fposition2, 1);
        globalPos = fposition2;
        EmitVertex();

        gl_Position = MVP * vec4(fposition3, 1);
        globalPos = fposition3;
        EmitVertex();

        gl_Position = MVP * vec4(fposition4.xyz, 1);
        globalPos = fposition4;
        EmitVertex();

        EndPrimitive();
	}
}
)TEXT";

const char *SPHERE_VS = R"TEXT(#version 330

uniform vec3 position;

void main()
{
	gl_Position = vec4(position.xyz, 1);
	//gl_Position = vec4(2, 2, 0, 1);
}

)TEXT";

const char *SPHERE_FS = R"TEXT(#version 330

layout(location = 0) out vec4 outputColor;
in vec3 globalPos;
uniform mat4 MVP;
uniform float radius;
uniform vec3 eyePosition;
uniform vec3 position;
uniform vec3 lightPos;
uniform vec4 color;

void main()
{
	float sphereRadius2 = radius * radius;

	vec3 rayDirection = normalize(globalPos - eyePosition);

	vec3 relSphCenter = position - eyePosition;

	float dt = dot(rayDirection, relSphCenter);

	vec3 cr = cross(rayDirection, relSphCenter);
	float d2 = dot(cr, cr);
	if (d2 > sphereRadius2)
        discard;

	float delta = sqrt(sphereRadius2 - d2);
	vec3 pointPos = eyePosition + ((dt - delta) > 0 ? (dt - delta) : (dt + delta)) * rayDirection;

	float wid = 0.03;
	vec3 pointPosMod = sign(mod(pointPos + vec3(wid, wid, wid) * 0.5, vec3(1, 1, 1)) - vec3(wid, wid, wid));
	float val = max(0.5, min(min(pointPosMod.x, pointPosMod.y), pointPosMod.z));

	float ang = dot(normalize(pointPos - position), normalize(lightPos - pointPos));
	outputColor = vec4((max(0, ang) + 0.3)*color.xyz*val, 1);

	vec4 realPos = MVP * vec4(pointPos, 1);

	float ndcDepth = realPos.z / realPos.w;
    gl_FragDepth = ((gl_DepthRange.diff * ndcDepth) +
        gl_DepthRange.near + gl_DepthRange.far) / 2.0;

}
)TEXT";

const char *SPHERE_GS = R"TEXT(#version 330

layout(points) in;
layout(triangle_strip, max_vertices = 24) out;

uniform mat4 MVP;
uniform float radius;

out vec3 globalPos;

const vec3 normals[6] = vec3[6](vec3(-1, 0, 0), vec3(1, 0, 0), vec3(0, -1, 0), vec3(0, 1, 0), vec3(0, 0, -1), vec3(0, 0, 1));
const vec3 t1s[6]     = vec3[6](vec3(0, -1, 0), vec3(0, 1, 0), vec3(1, 0, 0), vec3(-1, 0, 0), vec3(-1, 0, 0), vec3(1, 0, 0));
const vec3 t2s[6]     = vec3[6](vec3(0, 0, 1), vec3(0, 0, 1), vec3(0, 0, 1), vec3(0, 0, 1), vec3(0, 1, 0), vec3(0, 1, 0));

void main() {
    for (int i = 0; i < 6; ++i)
    {
        vec3 norm = normals[i] * radius;
        vec3 t1 = t1s[i] * radius;
        vec3 t2 = t2s[i] * radius;
        vec3 p1 = gl_in[0].gl_Position.xyz + norm;
        vec3 fposition1 = p1 - t1 - t2;
        vec3 fposition2 = p1 + t1 - t2;
        vec3 fposition3 = p1 - t1 + t2;
        vec3 fposition4 = p1 + t1 + t2;

        gl_Position = MVP * vec4(fposition1.xyz, 1);
        globalPos = fposition1;
        EmitVertex();

        gl_Position = MVP * vec4(fposition2, 1);
        globalPos = fposition2;
        EmitVertex();

        gl_Position = MVP * vec4(fposition3, 1);
        globalPos = fposition3;
        EmitVertex();

        gl_Position = MVP * vec4(fposition4.xyz, 1);
        globalPos = fposition4;
        EmitVertex();

        EndPrimitive();
	}
}
)TEXT";

NVGcolor toColor(uint32_t color) {
    return nvgRGBA((color & 0xFF000000) >> 24, (color & 0x00FF0000) >> 16, (color & 0x0000FF00) >> 8, color & 0x000000FF);
}

struct MeshData {
    GLuint vao = 0;
    GLuint quadBufferObject = 0;

    std::vector<float> data;
    GLenum usage = GL_STATIC_DRAW;

    void buildVao()
    {
        if (vao == 0)
            glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        if (quadBufferObject == 0)
            glGenBuffers(1, &quadBufferObject);
        glBindBuffer(GL_ARRAY_BUFFER, quadBufferObject);


        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), usage); //formatting the data for the buffer
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindVertexArray(0);
    }

    void pushV(const V3 &v)
    {
        data.push_back(v.x);
        data.push_back(v.y);
        data.push_back(v.z);
    }
};

struct GridData: MeshData
{
    void init(const P &minP, const P &maxP, double hMin, double hMax, double /*cellSize*/)
    {
        double z = 0.0;
        if (hMin > 0.0)
            z = hMin;

        data.clear();

        pushV(V3(minP.x, z, minP.y));
        pushV(V3(minP.x, z, maxP.y));

        pushV(V3(minP.x, z, maxP.y));
        pushV(V3(maxP.x, z, maxP.y));

        pushV(V3(maxP.x, z, maxP.y));
        pushV(V3(maxP.x, z, minP.y));

        pushV(V3(maxP.x, z, minP.y));
        pushV(V3(minP.x, z, minP.y));


        pushV(V3(minP.x, hMin, minP.y));
        pushV(V3(minP.x, hMax, minP.y));

        pushV(V3(minP.x, hMin, maxP.y));
        pushV(V3(minP.x, hMax, maxP.y));

        pushV(V3(maxP.x, hMin, maxP.y));
        pushV(V3(maxP.x, hMax, maxP.y));

        pushV(V3(maxP.x, hMin, minP.y));
        pushV(V3(maxP.x, hMax, minP.y));


        pushV(V3(minP.x, hMax, minP.y));
        pushV(V3(minP.x, hMax, maxP.y));

        pushV(V3(minP.x, hMax, maxP.y));
        pushV(V3(maxP.x, hMax, maxP.y));

        pushV(V3(maxP.x, hMax, maxP.y));
        pushV(V3(maxP.x, hMax, minP.y));

        pushV(V3(maxP.x, hMax, minP.y));
        pushV(V3(minP.x, hMax, minP.y));

        /*if (cellSize > 0)
        {
            for (double x = minP.x + cellSize; x < maxP.x; x += cellSize)
            {
                pushV(V3(x, z, minP.y));
                pushV(V3(x, z, maxP.y));
            }

            for (double y = minP.y + cellSize; y < maxP.y; y += cellSize)
            {
                pushV(V3(minP.x, z, y));
                pushV(V3(maxP.x, z, y));
            }
        }*/

        /*pushV(V3(-3000.0, 0, 0));
        pushV(V3(+3000.0, 0, 0));

        pushV(V3(0, -3000.0, 0));
        pushV(V3(0, +3000.0, 0));

        pushV(V3(0, 0, -3000.0));
        pushV(V3(0, 0, +3000.0));*/

        buildVao();
    }
};

struct DummyPointData: MeshData
{
    void init()
    {
        pushV(V3(0, 0, 0));
        buildVao();
    }
};

struct LineData : MeshData
{
    LineData() {
        usage = GL_STREAM_DRAW;
    }
};

void checkGLError(const std::string &errP)
{
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        std::cerr << "GL error " << errP << ": " << err;
    }
}

void setColor(GLint uniform, uint32_t color)
{
    float r = ((color & 0xFF000000) >> 24) / 255.0f;
    float g = ((color & 0x00FF0000) >> 16) / 255.0f;
    float b = ((color & 0x0000FF00) >> 8) / 255.0f;
    float a = (color & 0x000000FF) / 255.0f;

    glUniform4f(uniform, r, g, b, a);
}

struct ViewData
{
    ViewData(Settings &settings): gridShader("gridShader"), sphereShader("sphereShader"), diskShader("diskShader"),
        rectShader("rectShader"), cylInner025Shader("cylInner025Shader"), camera(settings) {}

    Shader gridShader, sphereShader, diskShader, rectShader, cylInner025Shader;
    Camera camera;
    GridData gridData;
    DummyPointData dummyPointData;
    LineData lineData;
    V3 vel = V3(0.0f, 0.0f, 0.0f);

    void initialize()
    {
        gridShader.buildShaderProgram(GRID_VS, GRID_FS);
        sphereShader.buildShaderProgram(SPHERE_VS, SPHERE_GS, SPHERE_FS);
        diskShader.buildShaderProgram(DISK_VS, DISK_GS, DISK_FS);
        rectShader.buildShaderProgram(RECT_VS, RECT_GS, RECT_FS);
        cylInner025Shader.buildShaderProgram(CYLINDER025_VS, CYLINDER025_GS, CYLINDER025_FS);

        dummyPointData.init();
    }

    void setupCommonUniforms(Shader &shader, const M4 &modelMat)
    {
        if (shader.uniforms.count("MVP"))
        {
            M4 MVP = camera.getVP() * modelMat;
            glUniformMatrix4fv(shader.uniforms["MVP"], 1, GL_FALSE, &MVP[0][0]);

            checkGLError("uniform MVP");
        }

        if (shader.uniforms.count("M"))
        {
            glUniformMatrix4fv(shader.uniforms["M"], 1, GL_FALSE, &modelMat[0][0]);

            checkGLError("uniform M");
        }

        if (shader.uniforms.count("MINV"))
        {
            M4 MINV = glm::inverse(modelMat);
            glUniformMatrix4fv(shader.uniforms["MINV"], 1, GL_FALSE, &MINV[0][0]);

            checkGLError("uniform M");
        }

        if (shader.uniforms.count("VP"))
        {
            M4 VP = camera.getVP();
            glUniformMatrix4fv(shader.uniforms["VP"], 1, GL_FALSE, &VP[0][0]);

            checkGLError("uniform VP");
        }

        if (shader.uniforms.count("MV"))
        {
            M4 MV = camera.getMatrix();
            glUniformMatrix4fv(shader.uniforms["MV"], 1, GL_FALSE, &MV[0][0]);

            checkGLError("uniform MV");
        }

        if (shader.uniforms.count("PROJ"))
        {
            M4 proj = camera.getProj();
            glUniformMatrix4fv(shader.uniforms["PROJ"], 1, GL_FALSE, &proj[0][0]);

            checkGLError("uniform PROJ");
        }

        if (shader.uniforms.count("eyePosition"))
        {
            glUniform3fv(shader.uniforms["eyePosition"], 1, &camera.position[0]);

            checkGLError("uniform eyePosition");
        }
    }

    void render3d(const StaticObjects *staticObjects, const Frame *frame)
    {
        glDisable(GL_BLEND);

        glEnable(GL_DEPTH_TEST);

        gridData.init(staticObjects->minP, staticObjects->maxP, staticObjects->hMin, staticObjects->hMax, staticObjects->cellSize);
        if (gridData.data.size() > 0)
        {
            glBindVertexArray(gridData.vao);
            checkGLError("bind vao");

            glUseProgram(gridShader.program);

            checkGLError("use program");

            setupCommonUniforms(gridShader, M4(1.0f));

            if (gridShader.uniforms.count("color"))
            {
                setColor(gridShader.uniforms["color"], 0x000000ff);

                checkGLError("uniform color");
            }

            glDrawArrays(GL_LINES, 0, gridData.data.size() / 3);

            checkGLError("draw");

            glBindVertexArray(0);
        }

        checkGLError("bind vao 0");

        for (const SObj &obj : staticObjects->objs)
            renderObj3d(obj);

        if (frame)
        {
            std::lock_guard<std::recursive_mutex> guard(frame->mutex);

            for (const Obj& obj : frame->objs)
            {
                for (const auto& p : obj.subObjs)
                {
                    renderObj3d(p.second);
                }
            }
        }
    }

    void renderObj3d(const SObj &sobj)
    {
        glDisable(GL_POLYGON_OFFSET_FILL);

        std::string type = getStr("type", sobj);
        if (type == "sphere")
        {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);

            V3 p = getV3("p", sobj);
            double rad = getDouble("r", sobj);
            uint32_t color = getInt("c", sobj);
            V3 lightPos = getV3("lp", sobj);

            glBindVertexArray(dummyPointData.vao);
            glUseProgram(sphereShader.program);

            setupCommonUniforms(sphereShader, M4(1.0f));

            if (sphereShader.uniforms.count("position"))
            {
                glUniform3fv(sphereShader.uniforms["position"], 1, &p[0]);

                checkGLError("uniform position");
            }

            if (sphereShader.uniforms.count("radius"))
            {
                glUniform1f(sphereShader.uniforms["radius"], rad);

                checkGLError("uniform radius");
            }

            if (sphereShader.uniforms.count("color"))
            {
                setColor(sphereShader.uniforms["color"], color);

                checkGLError("uniform color");
            }

            if (sphereShader.uniforms.count("lightPos"))
            {
                glUniform3fv(sphereShader.uniforms["lightPos"], 1, &lightPos[0]);

                checkGLError("uniform lightPos");
            }

            glDrawArrays(GL_POINTS, 0, 1);

            glBindVertexArray(0);
        }
        else if (type == "disk")
        {
            glDisable(GL_CULL_FACE);
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-1, -1);

            V3 p = getV3("p", sobj);
            V3 n = glm::normalize(getV3("n", sobj));
            double rad = getDouble("r", sobj);
            uint32_t color = getInt("c", sobj);

            glBindVertexArray(dummyPointData.vao);
            glUseProgram(diskShader.program);

            setupCommonUniforms(diskShader, M4(1.0f));

            if (diskShader.uniforms.count("position"))
            {
                glUniform3fv(diskShader.uniforms["position"], 1, &p[0]);

                checkGLError("uniform position");
            }

            if (diskShader.uniforms.count("normal"))
            {
                glUniform3fv(diskShader.uniforms["normal"], 1, &n[0]);

                checkGLError("uniform normal");
            }

            if (diskShader.uniforms.count("radius"))
            {
                glUniform1f(diskShader.uniforms["radius"], rad);

                checkGLError("uniform radius");
            }

            if (diskShader.uniforms.count("color"))
            {
                setColor(diskShader.uniforms["color"], color);

                checkGLError("uniform color");
            }

            glDrawArrays(GL_POINTS, 0, 1);

            glBindVertexArray(0);
        }
        else if (type == "rect")
        {
            glDisable(GL_CULL_FACE);

            V3 p = getV3("p", sobj);
            V3 ht1 = getV3("ht1", sobj);
            V3 ht2 = getV3("ht2", sobj);
            V3 gridShift = getV3("gridShift", sobj);
            uint32_t color = getInt("c", sobj);
            V3 lightPos = getV3("lp", sobj);

            glBindVertexArray(dummyPointData.vao);
            glUseProgram(rectShader.program);

            setupCommonUniforms(rectShader, M4(1.0f));

            if (rectShader.uniforms.count("position"))
            {
                glUniform3fv(rectShader.uniforms["position"], 1, &p[0]);

                checkGLError("uniform position");
            }

            if (rectShader.uniforms.count("ht1"))
            {
                glUniform3fv(rectShader.uniforms["ht1"], 1, &ht1[0]);

                checkGLError("uniform ht1");
            }

            if (rectShader.uniforms.count("ht2"))
            {
                glUniform3fv(rectShader.uniforms["ht2"], 1, &ht2[0]);

                checkGLError("uniform ht2");
            }

            if (rectShader.uniforms.count("gridShift"))
            {
                glUniform3fv(rectShader.uniforms["gridShift"], 1, &gridShift[0]);

                checkGLError("uniform gridShift");
            }

            if (rectShader.uniforms.count("color"))
            {
                setColor(rectShader.uniforms["color"], color);

                checkGLError("uniform color");
            }

            if (rectShader.uniforms.count("lightPos"))
            {
                glUniform3fv(rectShader.uniforms["lightPos"], 1, &lightPos[0]);

                checkGLError("uniform lightPos");
            }


            glDrawArrays(GL_POINTS, 0, 1);

            glBindVertexArray(0);
        }
        else if (type == "line3d")
        {
            uint32_t color = getInt("c", sobj);

            lineData.data.clear();

            for (int i = 1; i < 10000; ++i)
            {
                std::ostringstream oss;
                oss << "p" << i;
                if (sobj.count(oss.str()))
                {
                    V3 pi = getV3(oss.str(), sobj);
                    lineData.pushV(pi);
                }
                else
                {
                    break;
                }
            }

            if (!lineData.data.empty())
            {
                lineData.buildVao();

                glBindVertexArray(lineData.vao);
                checkGLError("bind vao");

                glUseProgram(gridShader.program);

                checkGLError("use program");

                setupCommonUniforms(gridShader, M4(1.0f));

                if (gridShader.uniforms.count("color"))
                {
                    setColor(gridShader.uniforms["color"], color);

                    checkGLError("uniform color");
                }

                glDrawArrays(GL_LINE_STRIP, 0, lineData.data.size() / 3);

                checkGLError("draw");

                glBindVertexArray(0);
            }
        }
        else if (type == "cyl_inner_025")
        {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);

            uint32_t color = getInt("c", sobj);
            V3 lightPos = getV3("lp", sobj);
            M4 modelMat = getM4("transf", sobj);

            glBindVertexArray(dummyPointData.vao);
            glUseProgram(cylInner025Shader.program);

            setupCommonUniforms(cylInner025Shader, modelMat);

            if (cylInner025Shader.uniforms.count("color"))
            {
                setColor(cylInner025Shader.uniforms["color"], color);

                checkGLError("uniform color");
            }

            if (cylInner025Shader.uniforms.count("lightPos"))
            {
                glUniform3fv(cylInner025Shader.uniforms["lightPos"], 1, &lightPos[0]);

                checkGLError("uniform lightPos");
            }

            glDrawArrays(GL_POINTS, 0, 1);

            glBindVertexArray(0);
        }
    }

    void processInput(const StaticObjects *staticObjects)
    {
        using namespace std;

        ImGuiIO& io = ImGui::GetIO();

        io.KeysDown;
        if (staticObjects->mode3d)
        {
            V3 accel = V3(0.0f, 0.0f, 0.0f);



            if (ImGui::IsKeyDown(GLFW_KEY_W)) {
                accel.y += 1.0;
            }

            if (ImGui::IsKeyDown(GLFW_KEY_S))
                accel.y -= 1.0;

            if (ImGui::IsKeyDown(GLFW_KEY_D))
                accel.x += 1.0;

            if (ImGui::IsKeyDown(GLFW_KEY_A))
                accel.x -= 1.0;

            if (ImGui::IsKeyDown(GLFW_KEY_SPACE))
                accel.z += 1.0;

            if (ImGui::IsKeyDown(GLFW_KEY_LEFT_CONTROL))
                accel.z -= 1.0;

            if (!ImGui::IsKeyDown(GLFW_KEY_W) && !ImGui::IsKeyDown(GLFW_KEY_S))
            {
                int sgn = (vel.y < 0.0f) ? -1 : ((vel.y > 0.0f) ? 1 : 0);
                accel.y = -sgn;
            }

            if (!ImGui::IsKeyDown(GLFW_KEY_D) && !ImGui::IsKeyDown(GLFW_KEY_A))
            {
                int sgn = (vel.x < 0.0f) ? -1 : ((vel.x > 0.0f) ? 1 : 0);
                accel.x = -sgn;
            }

            if (!ImGui::IsKeyDown(GLFW_KEY_SPACE) && !ImGui::IsKeyDown(GLFW_KEY_LEFT_CONTROL))
            {
                int sgn = (vel.z < 0.0f) ? -1 : ((vel.z > 0.0f) ? 1 : 0);
                accel.z = -sgn;
            }

            float maxVel = 50.0f;

            accel *= maxVel;

            float dt = 0.016;
            V3 oldVel = vel;
            vel += accel * dt;

            if (!ImGui::IsKeyDown(GLFW_KEY_W) && !ImGui::IsKeyDown(GLFW_KEY_S) && vel.y * oldVel.y < 0.0f)
                vel.y = 0.0f;

            if (!ImGui::IsKeyDown(GLFW_KEY_D) && !ImGui::IsKeyDown(GLFW_KEY_A) && vel.x * oldVel.x < 0.0f)
                vel.x = 0.0f;

            if (!ImGui::IsKeyDown(GLFW_KEY_SPACE) && !ImGui::IsKeyDown(GLFW_KEY_LEFT_CONTROL) && vel.z * oldVel.z < 0.0f)
                vel.z = 0.0f;

            if (glm::length(vel) > maxVel)
                vel *= (maxVel / glm::length(vel));

            glm::vec3 dPos = vel * dt;
            if (ImGui::IsKeyDown(GLFW_KEY_LEFT_SHIFT))
                dPos *= 10.0;

            dPos = camera.transform(dPos.y, dPos.x, dPos.z);
            camera.position += dPos;
            camera.settings.cameraPosition = camera.position;
        }
    }
};

void SceneRenderer::init() {
    vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    viewData->initialize();
}


void SceneRenderer::render(const StaticObjects *staticObjects, const Frame *frame) {
    if (staticObjects->mode3d)
    {
        viewData->render3d(staticObjects, frame);
    }
    else {
        nvgBeginFrame(vg, wid, heig, 1);
        nvgSave(vg);
        updateZoom();

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_SCISSOR_TEST);

        nvgStrokeColor(vg, nvgRGB(0, 0, 0));
        nvgStrokeWidth(vg, settings.lineThickness / zoom);
        nvgBeginPath(vg);
        nvgMoveTo(vg, 0, 0);
        nvgLineTo(vg, staticObjects->w, 0);
        nvgLineTo(vg, staticObjects->w, staticObjects->h);
        nvgLineTo(vg, 0, staticObjects->h);
        nvgLineTo(vg, 0, 0);
        nvgStroke(vg);

        int cellSize = staticObjects->cellSize;
        if (cellSize > 0 && cellSize / zoom < 0.2)
        {
            nvgStrokeColor(vg, nvgRGB(0.7,0.7,0.7));
            nvgStrokeWidth(vg, 0.5 / zoom);
            nvgBeginPath(vg);

            for (int x = cellSize; x < staticObjects->w; x += cellSize)
            {
                nvgMoveTo(vg, x, 0);
                nvgLineTo(vg, x, staticObjects->h);
            }

            for (int y = cellSize; y < staticObjects->w; y += cellSize)
            {
                nvgMoveTo(vg, 0, y);
                nvgLineTo(vg, staticObjects->w, y);
            }

            nvgStroke(vg);
        }

        for (const SObj &obj : staticObjects->objs)
            renderObj2d(obj, staticObjects->h);

        if (frame) {
            std::lock_guard<std::recursive_mutex> guard(frame->mutex);

            for (const Obj &obj : frame->objs) {
                for (const auto &p : obj.subObjs) {
                    renderObj2d(p.second, staticObjects->h);
                }
            }
        }

        if (rulerStarted) {

            P curPos = toLocalRelP(mousePos);
            double rad = rulerStart.dist(curPos);

            nvgStrokeColor(vg, toColor(0x000000ff));
            nvgBeginPath(vg);
            nvgEllipse(vg, rulerStart.x, rulerStart.y, rad, rad);
            nvgMoveTo(vg, rulerStart.x, rulerStart.y);
            nvgLineTo(vg, curPos.x, curPos.y);
            nvgStroke(vg);
        }

        nvgEndFrame(vg);
        nvgRestore(vg);
    }
}

void SceneRenderer::updateZoom() {
    P center = zoomCenter * P(wid, heig);
    P c1 = center - P(wid, heig) * 0.5 / zoom;
    P c2 = center + P(wid, heig) * 0.5 / zoom;

    zoomCenter = (c1 + c2) / 2.0 / P(wid, heig);

    nvgTransform(vg, zoom, 0, 0, zoom, -c1.x * zoom, -c1.y * zoom);
}

void SceneRenderer::renderObj2d(const SObj &sobj, double h) {
    std::string type = getStr("type", sobj);
    if (type == "circle")
    {
        P p = getP("p", sobj);
        double rad = getDouble("r", sobj);
        uint32_t color = getInt("c", sobj);

        nvgFillColor(vg, toColor(color));
        nvgBeginPath(vg);
        nvgEllipse(vg, p.x, transformY(p.y, h), rad, rad);
        nvgFill(vg);
    }
    if (type == "circumference")
    {
        P p = getP("p", sobj);
        double rad = getDouble("r", sobj);
        uint32_t color = getInt("c", sobj);

        nvgStrokeColor(vg, toColor(color));
        nvgBeginPath(vg);
        nvgEllipse(vg, p.x, transformY(p.y, h), rad, rad);
        nvgStroke(vg);
    }
    else if (type == "line")
    {
        P p1 = getP("p1", sobj);

        uint32_t color = getInt("c", sobj);

        nvgStrokeColor(vg, toColor(color));
        nvgBeginPath(vg);
        nvgMoveTo(vg, p1.x, transformY(p1.y, h));

        for (int i = 2; i < 10000; ++i)
        {
            std::ostringstream oss;
            oss << "p" << i;
            if (sobj.count(oss.str()))
            {
                P pi = getP(oss.str(), sobj);
                nvgLineTo(vg, pi.x, transformY(pi.y, h));
            }
            else
            {
                break;
            }
        }
        nvgStroke(vg);
    }
    else if (type == "poly")
    {
        P p1 = getP("p1", sobj);

        uint32_t color = getInt("c", sobj);

        nvgFillColor(vg, toColor(color));
        nvgBeginPath(vg);
        nvgMoveTo(vg, p1.x, transformY(p1.y, h));

        for (int i = 2; i < 10000; ++i)
        {
            std::ostringstream oss;
            oss << "p" << i;
            if (sobj.count(oss.str()))
            {
                P pi = getP(oss.str(), sobj);
                nvgLineTo(vg, pi.x, transformY(pi.y, h));
            }
            else
            {
                break;
            }
        }
        nvgFill(vg);
    }
    else if (type == "rects")
    {
        uint32_t color = getInt("c", sobj);
        nvgFillColor(vg, toColor(color));
        nvgStrokeColor(vg, toColor(color));

        double hw = getDouble("hw", sobj);
        if (hw == 0)
            hw = 1;

        double dwDef = getDouble("dw", sobj);
        if (dwDef == 0)
            dwDef = 0.45;

        for (int i = 0; i < 10000; ++i)
        {
            std::ostringstream oss;
            oss << "p" << i;
            if (sobj.count(oss.str()))
            {
                double dwOvr = getDouble("dw" + std::to_string(i), sobj);
                double dw = dwOvr > 0 ? dwOvr : dwDef;

                P pi = getP(oss.str(), sobj);

                nvgBeginPath(vg);
                nvgMoveTo(vg, (pi.x - 0.5) * hw, transformY((pi.y - 0.5) * hw, h));
                nvgLineTo(vg, (pi.x + 0.5) * hw, transformY((pi.y - 0.5) * hw, h));
                nvgLineTo(vg, (pi.x + 0.5) * hw, transformY((pi.y + 0.5) * hw, h));
                nvgLineTo(vg, (pi.x - 0.5) * hw, transformY((pi.y + 0.5) * hw, h));
                nvgLineTo(vg, (pi.x - 0.5) * hw, transformY((pi.y - 0.5) * hw, h));
                nvgStroke(vg);

                nvgBeginPath(vg);
                nvgMoveTo(vg, (pi.x - dw) * hw, transformY((pi.y - dw) * hw, h));
                nvgLineTo(vg, (pi.x + dw) * hw, transformY((pi.y - dw) * hw, h));
                nvgLineTo(vg, (pi.x + dw) * hw, transformY((pi.y + dw) * hw, h));
                nvgLineTo(vg, (pi.x - dw) * hw, transformY((pi.y + dw) * hw, h));
                nvgLineTo(vg, (pi.x - dw) * hw, transformY((pi.y - dw) * hw, h));
                nvgFill(vg);
            }
            else
            {
                break;
            }
        }
    }
}

void SceneRenderer::imguiInputProcessing(const StaticObjects *staticObjects) {
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureMouse)
    {
        if (io.MouseWheel != 0.0) {
            float w = io.MouseWheel / 8.0;
            P zoomPoint = zoomCenter + (mousePos - P(0.5, 0.5)) / zoom;

            if (w > 0) {
                for (int i = 0; i < w; ++i)
                    zoom *= 1.2;
            } else {
                for (int i = 0; i < -w; ++i)
                    zoom /= 1.2;
            }

            if (zoom < 0.1)
                zoom = 0.1;

            zoomCenter = zoomPoint - (mousePos - P(0.5, 0.5)) / zoom;
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right, false))
        {
            rulerStart = toLocalP(P(ImGui::GetMousePos().x, ImGui::GetMousePos().y));
            rulerStarted = !rulerStarted;
        }
    }

    P oldPos = mousePos;
    mousePos = P(ImGui::GetMousePos().x, ImGui::GetMousePos().y) / P(this->wid, this->heig);

    if (staticObjects->mode3d)
    {
        if (!io.WantCaptureMouse && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            P delta = (mousePos - oldPos) * P(this->wid, this->heig);
            viewData->camera.rotate(delta.x, delta.y);
        }

        if (!io.WantCaptureKeyboard)
        {
            viewData->processInput(staticObjects);
        }

        std::ostringstream oss;
        oss << "Pos " << viewData->camera.position.x << " " << viewData->camera.position.y << " " << viewData->camera.position.z;
        status = oss.str();
    }
    else
    {
        if (!io.WantCaptureMouse && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            P delta = mousePos - oldPos;
            zoomCenter -= delta / zoom;
        }

        P titlePos = (zoomCenter + (mousePos - P(0.5, 0.5))/zoom)*P(this->wid, this->heig);
        P clampedTitlePos = clampP(titlePos, P(0, 0), P(staticObjects->w, staticObjects->h));

        std::ostringstream oss;
        oss << "Pos " << clampedTitlePos.x << " " << transformY(clampedTitlePos.y, staticObjects->h) << " (" << titlePos.x << " " << transformY(titlePos.y, staticObjects->h) << ")";

        status = oss.str();

        if (rulerStarted)
        {
            P localP = toLocalRelP(mousePos);
            P delta = localP - rulerStart;
            oss.str("");
            oss.clear();

            int intDist = std::abs((int) localP.x - (int) rulerStart.x) + std::abs((int) localP.y - (int) rulerStart.y);
            oss << "Dist: " << delta.len() << " (" << delta.x << "," << (settings.yIsUp ? -delta.y : delta.y) << ": " << intDist << ")";

            rullerStatus = oss.str();
        }
        else
        {
            rullerStatus.clear();
        }
    }
}

SceneRenderer::SceneRenderer(Settings &settings) : settings(settings) {
    viewData = new ViewData(settings);
}

SceneRenderer::~SceneRenderer() {
    delete viewData;
}

void SceneRenderer::setWindowSize(int wid, int heig) {
    this->wid = wid;
    this->heig = heig;

    viewData->camera.aspectRatio = (double) wid / (double) heig;
}
