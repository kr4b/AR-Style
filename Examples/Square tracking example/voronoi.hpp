#include <cstddef>
#include <cstdlib>
#include <vector>
#include <utility>

#include <ARX/ARController.h>
#include <ARX/ARG/mtx.h>

#include "OBJ_Loader.h"
#include "utils.hpp"

#if HAVE_GL
#  if ARX_TARGET_PLATFORM_MACOS
#    include <OpenGL/gl.h>
#  else
#    include <GL/gl.h>
#  endif
#endif // HAVE_GL
#if HAVE_GLES2 || HAVE_GL3
#  include <ARX/ARG/mtx.h>
#  include <ARX/ARG/shader_gl.h>
#  if HAVE_GLES2
#    if ARX_TARGET_PLATFORM_IOS
#      include <OpenGLES/ES2/gl.h>
#    else
#      include <GLES2/gl2.h>
#    endif
#  else
#    if ARX_TARGET_PLATFORM_MACOS
#      include <OpenGL/gl3.h>
#    else
#      ifndef _WIN32
#			define GL_GLEXT_PROTOTYPES
#      endif
#      include "GL/glcorearb.h"
#    endif
#  endif
#endif // HAVE_GLES2 || HAVE_GL3

class Voronoi {
private:
    float transform[16];
    GLuint program;
    GLuint uniform;
    GLuint modelVAO;
    GLuint modelV3BO;
    GLuint modelIBO;
    GLuint modelCBO;
    GLuint modelO2BO;
    GLuint texture;

    objl::Mesh model;

    std::vector<float> vertices;
    std::vector<float> colors;
    std::vector<float> offsets;

    void loadProgram(ARG_API drawAPI) {
        const char vertShaderStringGLES2[] =
            "attribute vec3 vPosition;\n"
            "attribute vec3 vColor;\n"
            "attribute vec2 vOffset;\n"
            "uniform mat4 transform;\n"
            "out vec3 color;\n"
            "void main() {\n"
                "gl_Position = transform * vec4(vPosition, 1.0) + vec4(2.0 * vOffset.x - 1.0, 2.0 * vOffset.y - 1.0, 0.0, 0.0);\n"
                "color = vColor;\n"
            "}\n";
        const char fragShaderStringGLES2[] =
            "#ifdef GL_ES\n"
            "precision mediump float;\n"
            "#endif\n"
            "in vec3 color;\n"
            "void main() {\n"
                "gl_FragColor = vec4(color, 1.0);\n"
            "}\n";
        const char vertShaderStringGL3[] =
            "#version 150\n"
            "in vec3 vPosition;\n"
            "in vec3 vColor;\n"
            "in vec2 vOffset;\n"
            "uniform mat4 transform;\n"
            "out vec3 color;\n"
            "void main() {\n"
                "gl_Position = transform * vec4(vPosition, 1.0) + vec4(2.0 * vOffset.x - 1.0, 2.0 * vOffset.y - 1.0, 0.0, 0.0);\n"
                "color = vColor;\n"
            "}\n";
        const char fragShaderStringGL3[] =
            "#version 150\n"
            "out vec4 FragColor;\n"
            "in vec3 color;\n"
            "void main() {\n"
                "FragColor = vec4(color, 1.0);\n"
            "}\n";

        program =
            loadShaderProgram(NULL, drawAPI == ARG_API_GL3 ? vertShaderStringGL3 : vertShaderStringGLES2, NULL, drawAPI == ARG_API_GL3 ? fragShaderStringGL3 : fragShaderStringGLES2);

        glBindAttribLocation(program, 0, "vPosition");
        glBindAttribLocation(program, 1, "vColor");
        glBindAttribLocation(program, 2, "vOffset");

        uniform = glGetUniformLocation(program, "transform");
    }

    void loadGL(ARG_API drawAPI) {
        loadProgram(drawAPI);

        const char *resourcesDir = arUtilGetResourcesDirectoryPath(
            AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST);
        const std::string path = std::string(resourcesDir) + "/cone.obj";
        objl::Loader loader;
        loader.LoadFile(path);
        model = loader.LoadedMeshes[0];
        vertices.reserve(model.Vertices.size() * 3);
        for (int i = 0; i < model.Vertices.size(); i++) {
            objl::Vertex vertex = model.Vertices[i];
            vertices.insert(vertices.end(), {vertex.Position.X, vertex.Position.Y, vertex.Position.Z});
        }
#if HAVE_GL3 || HAVE_GLES2
        glGenTextures(1, &texture);

        glBindTexture(GL_TEXTURE_1D, texture);
        glTexImage1D(GL_TEXTURE_1D, 0, GL_RG32F, m * n, 0, GL_RG, GL_FLOAT, offsets.data());
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_1D, 0);
#endif

#if HAVE_GL3
        if (drawAPI == ARG_API_GL3) {
            glGenVertexArrays(1, &modelVAO);
            glBindVertexArray(modelVAO);
            glGenBuffers(1, &modelV3BO);
            glBindBuffer(GL_ARRAY_BUFFER, modelV3BO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(objl::Vector3) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
            glGenBuffers(1, &modelIBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelIBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * model.Indices.size(), model.Indices.data(), GL_STATIC_DRAW);

            // Indexing
            glGenBuffers(1, &modelCBO);
            glBindBuffer(GL_ARRAY_BUFFER, modelCBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * colors.size(), colors.data(), GL_STATIC_DRAW);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
            glVertexAttribDivisor(1, 1);

            glGenBuffers(1, &modelO2BO);
            glBindBuffer(GL_ARRAY_BUFFER, modelO2BO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * offsets.size(), offsets.data(), GL_STATIC_DRAW);
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);
            glVertexAttribDivisor(2, 1);
        }
#endif
    }

    void updateTexture() {
        glBindTexture(GL_TEXTURE_1D, texture);
        glTexSubImage1D(GL_TEXTURE_1D, 0, 0, m * n, GL_RG, GL_FLOAT, offsets.data());
        glBindTexture(GL_TEXTURE_1D, 0);
    }

public:
    const size_t m, n;
    // std::vector<std::pair<float, float>> centers;

    Voronoi(ARG_API drawAPI, size_t m, size_t n, int width, int height) : m(m), n(n) {
        // centers.resize(m * n);
        colors.resize(m * n * 3);
        offsets.resize(m * n * 2);
        const float scale = 4.0f / float(std::max(m, n));

        for (size_t i = 0; i < n; i++) {
            for (size_t j = 0; j < m; j++) {
                const float x = float(width) / float(m) * (j + float(rand()) / float(RAND_MAX));
                const float y = float(height) / float(n) * (i + float(rand()) / float(RAND_MAX));
                const size_t index = i * m + j;
                // centers[index] = std::make_pair(x, y);
                offsets[index * 2 + 0] = x / float(width);
                offsets[index * 2 + 1] = 1.0 - y / float(height);
                colors[index * 3 + 0] = float(index / (256 * 256)) / 255.0f;
                colors[index * 3 + 1] = float((index % (256 * 256)) / 256) / 255.0f;
                colors[index * 3 + 2] = float(index % 256) / 255.0f;
            }
        }

        mtxLoadIdentityf(transform);
        mtxRotatef(transform, -90.0f, 1.0f, 0.0f, 0.0f);
        mtxScalef(transform, scale, scale, scale);
        loadGL(drawAPI);
        // updateTexture();
    }

    void drawPattern() {
        glUseProgram(program);
        glUniformMatrix4fv(uniform, 1, GL_FALSE, transform);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

#if HAVE_GLES2
        if (drawAPI == ARG_API_GLES2) {
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices.data());
            glEnableVertexAttribArray(0);
        }
#else
        glBindVertexArray(modelVAO);
#endif
        glDrawArraysInstanced(GL_TRIANGLES, 0, model.Indices.size(), offsets.size() / 2);
#if HAVE_GL3
        glBindVertexArray(0);
#endif
    }

    void load(GLuint program) {
        glUseProgram(program);
        glUniform1i(glGetUniformLocation(program, "frame"), 0);
        glUniform1i(glGetUniformLocation(program, "pattern"), 1);
        glUniform1i(glGetUniformLocation(program, "centers"), 2);
    }

    void prepare(GLuint program, const float position[3], const float view[16], const float projection[16]) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_1D, texture);
        glUniform3fv(glGetUniformLocation(program, "highlight"), 1, position);
        glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, view);
        glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, projection);
    }
};
