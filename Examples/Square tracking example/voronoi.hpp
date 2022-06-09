#include <cstddef>
#include <cstdlib>
#include <vector>
#include <utility>

#include <ARX/ARController.h>
#include <ARX/ARG/mtx.h>

#include <opencv2/core.hpp>

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
#        define GL_GLEXT_PROTOTYPES
#      endif
#      include "GL/glcorearb.h"
#    endif
#  endif
#endif // HAVE_GLES2 || HAVE_GL3

class Voronoi {
private:
    float transform[16];
    GLuint program;
    GLuint densityProgram;
    GLuint uniforms[2];
    GLuint modelVAO;
    GLuint modelV3BO;
    GLuint modelIBO;
    GLuint modelO2BO;
    GLuint FBO;
    GLuint FBOTexture;
    objl::Mesh model;

    std::vector<float> vertices;
    std::vector<float> offsets;
    std::vector<float> worldPositions;

    const Voronoi* other;
    static constexpr float EPSILON = 0.2f;
    static constexpr float ALPHA = 0.01f;
    static constexpr float BETA = 0.9f;

    void loadPrograms(ARG_API drawAPI) {
        const char vertShaderStringGLES2[] =
            "attribute vec3 vPosition;\n"
            "attribute vec2 vOffset;\n"
            "uniform mat4 transform;\n"
            "uniform sampler2D frame;\n"
            "out vec3 color;\n"
            "void main() {\n"
                "gl_Position = transform * vec4(vPosition, 1.0) + vec4(2.0 * vOffset.x - 1.0, 2.0 * vOffset.y - 1.0, 0.0, 0.0);\n"
                "color = texture(frame, vOffset);\n"
            "}\n";
        const char fragShaderStringGLES2[] =
            "#ifdef GL_ES\n"
            "precision mediump float;\n"
            "#endif\n"
            "uniform bool density;\n"
            "in vec3 color;\n"
            "void main() {\n"
                "if (density) {\n"
                    "gl_FragColor = vec4(vec3(1.0), (1.0 - gl_FragCoord.z) * 0.5);\n"
                "} else {\n"
                    "gl_FragColor = vec4(color, 1.0);\n"
                "}\n"
            "}\n";
        const char vertShaderStringGL3[] =
            "#version 150\n"
            "in vec3 vPosition;\n"
            "in vec2 vOffset;\n"
            "uniform mat4 transform;\n"
            "uniform sampler2D frame;\n"
            "out vec3 color;\n"
            "void main() {\n"
                "gl_Position = transform * vec4(vPosition, 1.0) + vec4(2.0 * vOffset.x - 1.0, 2.0 * vOffset.y - 1.0, 0.0, 0.0);\n"
                "color = texture(frame, vOffset).xyz;\n"
            "}\n";
        const char fragShaderStringGL3[] =
            "#version 150\n"
            "out vec4 FragColor;\n"
            "uniform bool density;\n"
            "in vec3 color;\n"
            "void main() {\n"
                "if (density) {\n"
                    "FragColor = vec4(vec3(1.0), (1.0 - gl_FragCoord.z) * 0.5);\n"
                "} else {\n"
                    "FragColor = vec4(color, 1.0);\n"
                    // "if (gl_FragCoord.z < 0.52) {\n"
                    //     "FragColor = vec4(vec3(0.0), 1.0);\n"
                    // "} else {\n"
                    //     "FragColor = vec4(float(int((color.x + 0.1) * 57190) % 255) / 255.0, float(int((color.y + 0.1) * 751009) % 255) / 255.0, float(int((color.z + 0.1) * 83401) % 255) / 255.0, 1.0);\n"
                    // "}\n"
                "}\n"
            "}\n";

        program =
            loadShaderProgram(NULL, drawAPI == ARG_API_GL3 ? vertShaderStringGL3 : vertShaderStringGLES2, NULL, drawAPI == ARG_API_GL3 ? fragShaderStringGL3 : fragShaderStringGLES2);

        glBindAttribLocation(program, 0, "vPosition");
        glBindAttribLocation(program, 1, "vOffset");

        uniforms[0] = glGetUniformLocation(program, "transform");
        uniforms[1] = glGetUniformLocation(program, "density");
    }

    void loadGL(ARG_API drawAPI) {
        const float scale = 3.0f / float(std::max(m, n));
        mtxLoadIdentityf(transform);
        mtxRotatef(transform, -90.0f, 1.0f, 0.0f, 0.0f);
        mtxScalef(transform, scale, 0.5f, scale);

        loadPrograms(drawAPI);

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

        glGenFramebuffers(1, &FBO);
        glGenTextures(1, &FBOTexture);

        glBindTexture(GL_TEXTURE_2D, FBOTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m, n, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOTexture, 0);

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

            glGenBuffers(1, &modelO2BO);
            glBindBuffer(GL_ARRAY_BUFFER, modelO2BO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * m * n * 2 * 2, NULL, GL_STATIC_DRAW);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
            glVertexAttribDivisor(1, 1);
        }
#endif
    }

    cv::Vec3f getModelDepth(const std::vector<float>& modelDepth, float x, float y, int contentWidth, int contentHeight) {
        const size_t modelIndex = (int((1.0f - y) * float(contentHeight)) * contentWidth + int(x * float(contentHeight))) * 4;
        if (modelIndex > modelDepth.size()) {
            return cv::Vec3f(0.0f, 0.0f, 0.0f);
        }
        cv::Vec3f v = cv::Vec3f(
            modelDepth[modelIndex + 0],
            modelDepth[modelIndex + 1],
            modelDepth[modelIndex + 2]
        );
        return v;
    }

    void getWorldPosition(float x, float y, float worldPos[4], const cv::Mat& depth, const std::vector<float> &modelDepth, int width, int height, int contentWidth, int contentHeight, const float viewInv[16]) {
        cv::Vec3f v = getModelDepth(modelDepth, x, y, contentWidth, contentHeight);
        if (v[0] == 0.0f && v[1] == 0.0f && v[2] == 0.0f) {
            v = depth.at<cv::Vec3f>(int(y * float(height)), int(x * float(width)));
        }
        const float P[4] = {
            v[0],
            v[1],
            v[2],
            1.0f
        };
        transformVector(P, viewInv, worldPos);
    }

public:
    const size_t m, n;
    // std::vector<std::pair<float, float>> centers;

    Voronoi(ARG_API drawAPI, size_t m, size_t n, int width, int height) : m(m), n(n), other(NULL) {
        // centers.resize(m * n);
        offsets.resize(m * n * 2);

        for (size_t i = 0; i < n; i++) {
            for (size_t j = 0; j < m; j++) {
                const float x = (j + float(rand()) / float(RAND_MAX)) / float(m);
                const float y = (i + float(rand()) / float(RAND_MAX)) / float(n);
                const size_t index = i * m + j;
                // centers[index] = std::make_pair(x, y);
                offsets[index * 2 + 0] = x;
                offsets[index * 2 + 1] = (1.0f - y);
            }
        }

        loadGL(drawAPI);

        glBindBuffer(GL_ARRAY_BUFFER, modelO2BO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * offsets.size(), offsets.data());
    }

    Voronoi(ARG_API drawAPI, const Voronoi* other) : m(other->m), n(other->n), other(other) {
        offsets.resize(other->offsets.size());

        loadGL(drawAPI);
    }

    void updateDepth(const cv::Mat& depth, const std::vector<float> &modelDepth, const float view[16], const float projection[16], int width, int height, int contentWidth, int contentHeight) {
        float viewInv[16];
        invertMatrix(view, viewInv);
        // float projInv[16];
        // invertMatrix(projection, projInv);

        if (worldPositions.empty()) {
            if (other) {
                worldPositions = other->worldPositions;
            } else {
                worldPositions.resize(m * n * 3);
                for (size_t i = 0; i < n; i++) {
                    for (size_t j = 0; j < m; j++) {
                        const size_t index = i * m + j;
                        const float x = offsets[index * 2 + 0];
                        const float y = (1.0f - offsets[index * 2 + 1]);
                        float worldPos[4];
                        getWorldPosition(x, y, worldPos, depth, modelDepth, width, height, contentWidth, contentHeight, viewInv);
                        worldPositions[index * 3 + 0] = worldPos[0];
                        worldPositions[index * 3 + 1] = worldPos[1];
                        worldPositions[index * 3 + 2] = worldPos[2];
                    }
                }
            }
        }

        std::vector<float> newOffsets, newWorldPositions;
        newOffsets.reserve(m * n * 2);
        newWorldPositions.reserve(m * n * 3);
        for (size_t i = 0; i < offsets.size() / 2; i++) {
            const float worldPos[4] = {
                worldPositions[i * 3 + 0],
                worldPositions[i * 3 + 1],
                worldPositions[i * 3 + 2],
                1.0f,
            };
            float viewPos[4];
            transformVector(worldPos, view, viewPos);

            for (int j = 0; j < 4; j++) {
                viewPos[j] /= viewPos[3];
            }

            float imagePos[4];
            transformVector(viewPos, projection, imagePos);

            for (int j = 0; j < 4; j++) {
                imagePos[j] /= imagePos[3];
            }

            const float x = (imagePos[0] + 1.0f) / 2.0f;
            const float y = 1.0f - (imagePos[1] + 1.0f) / 2.0f;
            if (x < 0.0f || y < 0.0f || x >= 1.0f || y >= 1.0f) {
                continue;
            }

            cv::Vec3f v = getModelDepth(modelDepth, x, y, contentWidth, contentHeight);
            if (v[0] == 0.0f && v[1] == 0.0f && v[2] == 0.0f) {
                v = depth.at<cv::Vec3f>(int(y * float(height)), int(x * float(width)));
            }
            // TODO: Determine required accuracy for depth comparison
            if (cv::norm(v, cv::Vec3f(viewPos), cv::NORM_L2) <= EPSILON) {
                newOffsets.push_back(x);
                newOffsets.push_back(1.0f - y);
                for (int j = 0; j < 3; j++) {
                    newWorldPositions.push_back(worldPositions[i * 3 + j]);
                }
            }
        }

        offsets = newOffsets;
        worldPositions = newWorldPositions;

        glBindBuffer(GL_ARRAY_BUFFER, modelO2BO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * offsets.size(), offsets.data());
    }

    void updateDensity(GLuint program, const cv::Mat& depth, const std::vector<float> &modelDepth, const float view[16], int width, int height, int contentWidth, int contentHeight) {
        float viewInv[16];
        invertMatrix(view, viewInv);

        glViewport(0, 0, m, n);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUniform1i(glGetUniformLocation(program, "cellWidth"), int(contentWidth / m));
        glUniform1i(glGetUniformLocation(program, "cellHeight"), int(contentHeight / n));
        glDrawArrays(GL_TRIANGLES, 0, 6);

        std::vector<unsigned char> density(m * n * 3);
        glReadPixels(0, 0, m, n, GL_RGB, GL_UNSIGNED_BYTE, density.data());

        const float mInv = 1.0f / float(m);
        const float nInv = 1.0f / float(n);

        for (size_t i = 0; i < n; i++) {
            for (size_t j = 0; j < m; j++) {
                const float d = float(density[((n - i - 1) * m + j) * 3]) / 255.0f;
                if (d > ALPHA && d < BETA) {
                    continue;
                }
                if (d >= BETA) {
                    for (int k = offsets.size() - 2; k >= 0; k -= 2) {
                        const float x = offsets[k + 0];
                        const float y = 1.0f - offsets[k + 1];
                        if (x >= float(j) * mInv && y >= float(i) * nInv && x < float(j + 1) * mInv && y < float(i + 1) * nInv) {
                            offsets.erase(offsets.begin() + k, offsets.begin() + k + 2);
                        }
                    }
                }
                const float x = (j + float(rand()) / float(RAND_MAX)) / float(m);
                const float y = (i + float(rand()) / float(RAND_MAX)) / float(n);
                offsets.push_back(x);
                offsets.push_back(1.0f - y);
                float worldPos[4];
                getWorldPosition(x, y, worldPos, depth, modelDepth, width, height, contentWidth, contentHeight, viewInv);
                for (int i = 0; i < 3; i++) {
                    worldPositions.push_back(worldPos[i]);
                }
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, modelO2BO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * offsets.size(), offsets.data());
    }

    void drawPattern(bool density) {
        glUseProgram(program);
        glUniformMatrix4fv(uniforms[0], 1, GL_FALSE, transform);
        glUniform1i(uniforms[1], density);
        if (density) {
            glDisable(GL_DEPTH_TEST);
        } else {
            glEnable(GL_DEPTH_TEST);
        }
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

    // void load(GLuint program) {
    //     glUseProgram(program);
    //     glUniform1i(glGetUniformLocation(program, "frame"), 0);
    //     glUniform1i(glGetUniformLocation(program, "pattern"), 1);
    //     glUniform1i(glGetUniformLocation(program, "centers"), 2);
    // }

    // void prepare(GLuint program, const float position[3], const float view[16], const float projection[16], bool highlight) {
    //     glActiveTexture(GL_TEXTURE2);
    //     glBindTexture(GL_TEXTURE_1D, texture);
    //     glUniform3fv(glGetUniformLocation(program, "position"), 1, position);
    //     glUniform1i(glGetUniformLocation(program, "highlight"), highlight);
    //     glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, view);
    //     glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, projection);
    // }
};
