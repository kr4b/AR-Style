//
//  draw.cpp
//  artoolkitX Square Tracking Example
//
//  Copyright 2018 Realmax, Inc. All Rights Reserved.
//
//  Author(s): Philip Lamb
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//  this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright
//  notice, this list of conditions and the following disclaimer in the
//  documentation and/or other materials provided with the distribution.
//
//  3. Neither the name of the copyright holder nor the names of its
//  contributors may be used to endorse or promote products derived from this
//  software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//  POSSIBILITY OF SUCH DAMAGE.
//

#include <fstream>
#include <string>
#include <iostream>

#include "draw.h"
#include "OBJ_Loader.h"
#include "utils.hpp"
#include "voronoi.hpp"

#include <ARX/ARController.h>

#include <opencv2/core.hpp>
#include <opencv2/core/traits.hpp>
#include <opencv2/stereo.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/ximgproc/disparity_filter.hpp>

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

#define DRAW_MODELS_MAX 32
#define BASELINE false

#if HAVE_GLES2 || HAVE_GL3
// Indices of GL program uniforms.
enum {
    UNIFORM_VIEW_MATRIX,
    UNIFORM_MODEL_MATRIX,
    UNIFORM_PROJECTION_MATRIX,
    UNIFORM_DIFFUSE_COLOR,
    UNIFORM_CAMERA_POSITION,
    UNIFORM_WIDTH,
    UNIFORM_HEIGHT,
    UNIFORM_COUNT
};
// Indices of of GL program attributes.
enum {
    ATTRIBUTE_VERTEX,
    ATTRIBUTE_NORMAL,
    ATTRIBUTE_COUNT
};
static GLint uniforms[2][UNIFORM_COUNT] = {0};
static GLuint program = 0;
static GLuint depthProgram = 0;
static GLuint postProgram = 0;

#if HAVE_GL3
static GLuint gModelVAO = 0;
static GLuint gModelV3BO = 0;
static GLuint gModelN3BO = 0;
static GLuint gModelIBO = 0;
static GLuint gQuadVAO = 0;
static GLuint gQuadVBOs[2] = {0};

static GLuint gFBOs[3] = {0};
static GLuint gFBOTextures[3] = {0};
static GLuint gRBOs[2] = {0};

	#if defined(_WIN32)
	# define ARGL_GET_PROC_ADDRESS wglGetProcAddress
	PFNGLBINDBUFFERPROC glBindBuffer = NULL; // (PFNGLGENBUFFERSPROC)ARGL_GET_PROC_ADDRESS("glGenBuffersARB");
	PFNGLDELETEBUFFERSPROC glDeleteBuffers = NULL;
	PFNGLGENBUFFERSPROC glGenBuffers = NULL;
	PFNGLBUFFERDATAPROC glBufferData = NULL;
	PFNGLATTACHSHADERPROC glAttachShader = NULL;
	PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation = NULL;
	PFNGLCREATEPROGRAMPROC glCreateProgram = NULL;
	PFNGLDELETEPROGRAMPROC glDeleteProgram = NULL;
	PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = NULL;
	PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = NULL;
	PFNGLUSEPROGRAMPROC glUseProgram = NULL;

	PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = NULL;
	PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = NULL;
	PFNGLBINDVERTEXARRAYPROC glBindVertexArray = NULL;
	PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = NULL;
	PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = NULL;

    PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = NULL;
    PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = NULL;
    PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = NULL;
    PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = NULL;

    PFNGLACTIVETEXTUREPROC glActiveTexture = NULL;
    PFNGLGENTEXTURESPROC glGenTextures = NULL;
    PFNGLBINDTEXTUREPROC glBindTexture = NULL;
    PFNGLTEXIMAGE2DPROC glTexImage2D = NULL;
    PFNGLTEXPARAMETERIPROC glTexParameteri = NULL;
    PFNGLDELETETEXTURESPROC glDeleteTextures = NULL;

    PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = NULL;
    PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = NULL;
    PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = NULL;
    PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = NULL;
    PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers = NULL;
	#endif
#endif // HAVE_GL3

#endif // HAVE_GLES2 || HAVE_GL3

static ARG_API drawAPI = ARG_API_None;
static bool rotate90 = false;
static bool flipH = false;
static bool flipV = false;
static bool visible = true;

static int32_t gViewport[4] = {0};
static float gProjection[16];
static float gView[16];
static bool gModelLoaded[DRAW_MODELS_MAX] = {false};
static float gModelPoses[DRAW_MODELS_MAX][16];
static bool gModelVisbilities[DRAW_MODELS_MAX];

static objl::Mesh model;
static std::vector<float> mVertices;
static std::vector<float> mNormals;
static Voronoi* voronoi[2];
static std::vector<cv::Vec3f> worldPositions[2];
static std::vector<cv::Vec3b> colors[2];

static void drawModel(float pose[16], size_t uniform_index);
static void drawPost(size_t index, const float pose[16], const std::vector<float>& modelDepth, int width, int height, int contentWidth, int contentHeight);
static cv::Ptr<cv::StereoMatcher> stereoLeft, stereoRight;
static cv::Ptr<cv::ximgproc::DisparityWLSFilter> wlsFilterLeft, wlsFilterRight;
static cv::Mat leftDepth, rightDepth;
static float position[3] = {0};
static int mouseX = 0, mouseY = 0;

void drawInit() {
    // Load virtual model
    const char *resourcesDir = arUtilGetResourcesDirectoryPath(
        AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST);
    const std::string path = std::string(resourcesDir) + "/shrek.obj";
    objl::Loader loader;
    loader.LoadFile(path);
    model = loader.LoadedMeshes[0];
    mVertices.reserve(model.Vertices.size() * 3);
    mNormals.reserve(model.Vertices.size() * 3);
    for (int i = 0; i < model.Vertices.size(); i++) {
        objl::Vertex vertex = model.Vertices[i];
        mVertices.insert(mVertices.end(), {vertex.Position.X, vertex.Position.Y, vertex.Position.Z});
        mNormals.insert(mNormals.end(), {vertex.Normal.X, vertex.Normal.Y, vertex.Normal.Z});
    }

    // Create stereo matchers and filters
    stereoLeft = cv::StereoSGBM::create(0, 64, 9);
    wlsFilterLeft = cv::ximgproc::createDisparityWLSFilter(stereoLeft);
    stereoRight = cv::ximgproc::createRightMatcher(stereoLeft);
    wlsFilterRight = cv::ximgproc::createDisparityWLSFilter(stereoRight);
}

void drawToggleModels() {
    visible = !visible;
}

void drawSetup(ARG_API drawAPI_in, bool rotate90_in, bool flipH_in, bool flipV_in, int width, int height) {
#if defined(_WIN32)
	if (!glBindBuffer) glBindBuffer = (PFNGLBINDBUFFERPROC)ARGL_GET_PROC_ADDRESS("glBindBuffer");
	if (!glDeleteBuffers) glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)ARGL_GET_PROC_ADDRESS("glDeleteBuffers");
	if (!glGenBuffers) glGenBuffers = (PFNGLGENBUFFERSPROC)ARGL_GET_PROC_ADDRESS("glGenBuffers");
	if (!glBufferData) glBufferData = (PFNGLBUFFERDATAPROC)ARGL_GET_PROC_ADDRESS("glBufferData");
	if (!glAttachShader) glAttachShader = (PFNGLATTACHSHADERPROC)ARGL_GET_PROC_ADDRESS("glAttachShader");

	if (!glBindAttribLocation) glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)ARGL_GET_PROC_ADDRESS("glBindAttribLocation");
	if (!glCreateProgram) glCreateProgram = (PFNGLCREATEPROGRAMPROC)ARGL_GET_PROC_ADDRESS("glCreateProgram");
	if (!glDeleteProgram) glDeleteProgram = (PFNGLDELETEPROGRAMPROC)ARGL_GET_PROC_ADDRESS("glDeleteProgram");
	if (!glEnableVertexAttribArray) glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)ARGL_GET_PROC_ADDRESS("glEnableVertexAttribArray");
	if (!glGetUniformLocation) glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)ARGL_GET_PROC_ADDRESS("glGetUniformLocation");
	if (!glUseProgram) glUseProgram = (PFNGLUSEPROGRAMPROC)ARGL_GET_PROC_ADDRESS("glUseProgram");

	if (!glUniformMatrix4fv) glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)ARGL_GET_PROC_ADDRESS("glUniformMatrix4fv");
	if (!glVertexAttribPointer) glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)ARGL_GET_PROC_ADDRESS("glVertexAttribPointer");
	if (!glBindVertexArray) glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)ARGL_GET_PROC_ADDRESS("glBindVertexArray");
	if (!glDeleteVertexArrays) glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)ARGL_GET_PROC_ADDRESS("glDeleteVertexArrays");
	if (!glGenVertexArrays) glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)ARGL_GET_PROC_ADDRESS("glGenVertexArrays");

    if (!glGenFramebuffers) glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)ARGL_GET_PROC_ADDRESS("glGenFramebuffers");
    if (!glBindFramebuffer) glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)ARGL_GET_PROC_ADDRESS("glBindFramebuffer");
    if (!glFramebufferTexture2D) glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)ARGL_GET_PROC_ADDRESS("glFramebufferTexture2D");
    if (!glDeleteFramebuffers) glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)ARGL_GET_PROC_ADDRESS("glDeleteFramebuffers");

    if (!glActiveTexture) glActiveTexture = (PFNGLACTIVETEXTUREPROC)ARGL_GET_PROC_ADDRESS("glActiveTexture");
    if (!glGenTextures) glGenTextures = (PFNGLGENTEXTURESPROC)ARGL_GET_PROC_ADDRESS("glGenTextures");
    if (!glBindTexture) glBindTexture = (PFNGLBINDTEXTUREPROC)ARGL_GET_PROC_ADDRESS("glBindTexture");
    if (!glTexImage2D) glTexImage2D = (PFNGLTEXIMAGE2DPROC)ARGL_GET_PROC_ADDRESS("glTexImage2D");
    if (!glTexParameteri) glTexParameteri = (PFNGLTEXPARAMETERIPROC)ARGL_GET_PROC_ADDRESS("glTexParameteri");
    if (!glDeleteTextures) glDeleteTextures = (PFNGLDELETETEXTURESPROC)ARGL_GET_PROC_ADDRESS("glDeleteTextures");

    if (!glGenRenderbuffers) glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)ARGL_GET_PROC_ADDRESS("glGenRenderbuffers");
    if (!glBindRenderbuffer) glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)ARGL_GET_PROC_ADDRESS("glBindRenderbuffer");
    if (!glRenderbufferStorage) glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)ARGL_GET_PROC_ADDRESS("glRenderbufferStorage");
    if (!glFramebufferRenderbuffer) glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)ARGL_GET_PROC_ADDRESS("glFramebufferRenderbuffer");
    if (!glDeleteRenderbuffers) glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)ARGL_GET_PROC_ADDRESS("glDeleteRenderbuffers");

    if (!glBindBuffer || !glDeleteBuffers || !glGenBuffers || !glBufferData ||
        !glAttachShader || !glBindAttribLocation || !glCreateProgram || !glDeleteProgram ||
        !glEnableVertexAttribArray || !glGetUniformLocation || !glUseProgram || !glUniformMatrix4fv ||
        !glVertexAttribPointer || !glBindVertexArray || !glDeleteVertexArrays || !glGenVertexArrays ||
        !glGenFramebuffers || !glBindFramebuffer || !glFramebufferTexture2D || !glDeleteFramebuffers ||
        !glActiveTexture || !glGenTextures || !glBindTexture || !glTexImage2D || !glTexParameteri ||
        !glDeleteTextures || !glGenRenderbuffers || !glBindRenderbuffer || !glRenderbufferStorage ||
        !glFramebufferRenderbuffer || !glDeleteRenderbuffers) {
        ARLOGe("Error: a required OpenGL function counld not be bound.\n");
    }
#endif

    drawAPI = drawAPI_in;
    rotate90 = rotate90_in;
    flipH = flipH_in;
    flipV = flipV_in;

    // Load Voronoi instances (swap bottom two for baseline)
    voronoi[0] = new Voronoi(drawAPI, 80, 80, width, height);
#if BASELINE
    voronoi[1] = new Voronoi(drawAPI, 80, 80, width, height);
#else
    voronoi[1] = new Voronoi(drawAPI, voronoi[0]);
#endif

#if HAVE_GLES2 || HAVE_GL3
    glActiveTexture(GL_TEXTURE0);
    // Create framebuffers
    glGenFramebuffers(3, gFBOs);

    // Create framebuffer textures
    glGenTextures(3, gFBOTextures);

    // Create render buffers
    glGenRenderbuffers(2, gRBOs);

    for (int j = 0; j <= 1; j++) {
        glBindTexture(GL_TEXTURE_2D, gFBOTextures[j]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindRenderbuffer(GL_RENDERBUFFER, gRBOs[j]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

        glBindFramebuffer(GL_FRAMEBUFFER, gFBOs[j]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gFBOTextures[j], 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gRBOs[j]);
    }

    glBindTexture(GL_TEXTURE_2D, gFBOTextures[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, gFBOs[2]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gFBOTextures[2], 0);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif

    return;
}

void drawUpdate(int width, int height, int contentWidth, int contentHeight, std::vector<unsigned char> frames[2]) {
    // auto start = std::chrono::system_clock::now();
    // if (depth.elemSize() > 0) {
    //     return;
    // }
    cv::Mat left(width, height, CV_8UC4, frames[0].data());
    cv::Mat right(width, height, CV_8UC4, frames[1].data());
    cv::Mat leftGray, rightGray, leftDisparity, rightDisparity, leftFilteredDisparity, rightFilteredDisparity;

    // Image preprocessing
    cv::cvtColor(left, left, cv::COLOR_RGBA2RGB);
    cv::cvtColor(right, right, cv::COLOR_RGBA2RGB);
    cv::cvtColor(left, leftGray, cv::COLOR_RGB2GRAY);
    cv::cvtColor(right, rightGray, cv::COLOR_RGB2GRAY);
    cv::resize(leftGray, leftGray, cv::Size(), 0.5, 0.5, cv::INTER_LINEAR_EXACT);
    cv::resize(rightGray, rightGray, cv::Size(), 0.5, 0.5, cv::INTER_LINEAR_EXACT);

    // Stereo matching
    stereoLeft->compute(leftGray, rightGray, leftDisparity);
    stereoRight->compute(rightGray, leftGray, rightDisparity);
    leftDisparity.convertTo(leftDisparity, CV_32F, 1.0);
    rightDisparity.convertTo(rightDisparity, CV_32F, 1.0);

    leftDisparity = (leftDisparity / 16.0f);
    rightDisparity = (rightDisparity / 16.0f);

    // Disparity filtering
    wlsFilterLeft->filter(leftDisparity, left, leftFilteredDisparity, rightDisparity);
    wlsFilterRight->filter(rightDisparity, right, rightFilteredDisparity, leftDisparity);

    // Save disparity maps
    // cv::imwrite("disparityFL.jpg", leftFilteredDisparity);
    // cv::imwrite("disparityFR.jpg", -rightFilteredDisparity);
    // cv::imwrite("disparityL.jpg", leftDisparity);
    // cv::imwrite("disparityR.jpg", -rightDisparity);

    // Convert disparity to view space map
    float focal = 1024.0f / 2.0f / tan(45.0f / 180.0f * M_PI / 2.0f);
    float baseline = 0.055f;
    float QData[16] = {
        -1.0f, 0.0f, 0.0f, float(width) / 2.0f,
        0.0f, 1.0f, 0.0f, -float(height) / 2.0f,
        0.0f, 0.0f, 0.0f, focal,
        0.0f, 0.0f, 0.895f * -1.0f / baseline, 0.0f
    };
    cv::Mat Q(4, 4, CV_32F, QData);
    cv::reprojectImageTo3D(leftFilteredDisparity, leftDepth, Q);
    cv::reprojectImageTo3D(-rightFilteredDisparity, rightDepth, Q);
    // depth = focal * baseline / disparity;

    // auto now = std::chrono::system_clock::now();
    // auto elapsed = now - start;
    // printf("Disparity: %d\n", std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
}

// Prepare framebuffer for rendering of real image
void drawPrepare() {
    glBindFramebuffer(GL_FRAMEBUFFER, gFBOs[0]);
    glViewport(0, 0, gViewport[2] / 2, gViewport[3]);
}

void drawCleanup() {
#if HAVE_GLES2 || HAVE_GL3
    if (drawAPI == ARG_API_GLES2 || drawAPI == ARG_API_GL3) {
        if (program) {
            glDeleteProgram(program);
            program = 0;
        }
#if HAVE_GL3
        if (drawAPI == ARG_API_GL3) {
            if (gModelVAO) {
                glDeleteBuffers(1, &gModelIBO);
                glDeleteBuffers(1, &gModelN3BO);
                glDeleteBuffers(1, &gModelV3BO);
                glDeleteVertexArrays(1, &gModelVAO);
                gModelVAO = 0;
            }
            if (gQuadVAO) {
                glDeleteBuffers(2, gQuadVBOs);
                glDeleteVertexArrays(1, &gQuadVAO);
                glDeleteRenderbuffers(2, gRBOs);
                glDeleteTextures(2, gFBOTextures);
                glDeleteFramebuffers(2, gFBOs);
            }
        }
#endif // HAVE_GL3
    }
#endif // HAVE_GLES2 || HAVE_GL3
    for (int i = 0; i < DRAW_MODELS_MAX; i++) gModelLoaded[i] = false;
    for (int i = 0; i <= 1; i++) delete voronoi[i];

    return;
}

int drawLoadModel() {
    for (int i = 0; i < DRAW_MODELS_MAX; i++) {
        if (!gModelLoaded[i]) {
            gModelLoaded[i] = true;
            return i;
        }
    }
    return -1;
}

void drawSetViewport(int32_t viewport[4]) {
    gViewport[0] = viewport[0];
    gViewport[1] = viewport[1];
    gViewport[2] = viewport[2];
    gViewport[3] = viewport[3];
}

void drawSetCamera(float projection[16], float view[16]) {
    if (projection) {
        if (flipH || flipV) {
            mtxLoadIdentityf(gProjection);
            mtxScalef(gProjection, flipV ? -1.0f : 1.0f,  flipH ? -1.0f : 1.0f, 1.0f);
            mtxMultMatrixf(gProjection, projection);
        } else {
            mtxLoadMatrixf(gProjection, projection);
        }
    } else {
        mtxLoadIdentityf(gProjection);
    }
    if (view) {
        mtxLoadMatrixf(gView, view);
    } else {
        mtxLoadIdentityf(gView);
    }
}

void drawSetModel(int modelIndex, bool visible, float pose[16]) {
    if (modelIndex < 0 || modelIndex >= DRAW_MODELS_MAX) return;
    if (!gModelLoaded[modelIndex]) return;
    
    gModelVisbilities[modelIndex] = visible;
    if (visible) mtxLoadMatrixf(&(gModelPoses[modelIndex][0]), pose);
}

void draw(size_t index, int width, int height, int contentWidth, int contentHeight) {
    glViewport(0, 0, contentWidth, contentHeight);

#if HAVE_GL
    if (drawAPI == ARG_API_GL) {
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(gProjection);
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(gView);
    }
#endif

#if HAVE_GLES2 || HAVE_GL3
    if (drawAPI == ARG_API_GLES2 || drawAPI == ARG_API_GL3) {
        if (!program) {
            GLuint vertShader = 0, fragShader = 0;
            // A simple shader pair which accepts just a vertex position and colour, no lighting.
            const char vertShaderStringGLES2[] =
                "attribute vec3 position;\n"
                "attribute vec3 vNormal;\n"
                "uniform mat4 model;\n"
                "uniform mat4 view;\n"
                "uniform mat4 projection;\n"
                "out vec3 normal;\n"
                "out vec3 position;\n"
                "void main() {\n"
                    "vec4 modelPosition = model * vec4(vPosition, 1.0);\n"
                    "gl_Position = projection * view * modelPosition;\n"
                    "normal = normalize((model * vec4(vNormal, 0.0)).xyz);\n"
                    "position = modelPosition.xyz;\n"
                "}\n";
            const char fragShaderStringGLES2[] =
                "#ifdef GL_ES\n"
                "precision mediump float;\n"
                "#endif\n"
                "uniform vec3 diffuseColor;\n"
                "uniform vec3 cameraPos;\n"
                "uniform mat4 view;\n"
                "in vec3 normal;\n"
                "in vec3 position;\n"
                "void main() {\n"
                    "vec3 lightPos = (vec4(-5.0, 0.0, -8.0, 1.0)).xyz;\n"
                    "vec3 lightDir = normalize(position - lightPos);\n"
                    "vec3 viewDir = normalize(position - cameraPos);\n "
                    "vec3 reflection = reflect(lightDir, normal);\n"
                    "float specular = clamp(dot(reflection, viewDir), 0.0, 1.0);\n"
                    "float intensity = clamp(dot(normal, lightDir), 0.0, 1.0);\n"
                    "gl_FragColor = vec4(intensity * diffuseColor + pow(specular, 15) * vec3(1.0), 1.0);\n"
                "}\n";
            const char vertShaderStringGL3[] =
                "#version 150\n"
                "in vec3 vPosition;\n"
                "in vec3 vNormal;\n"
                "uniform mat4 model;\n"
                "uniform mat4 view;\n"
                "uniform mat4 projection;\n"
                "out vec3 normal;\n"
                "out vec3 position;\n"
                "void main() {\n"
                    "vec4 modelPosition = model * vec4(vPosition, 1.0);\n"
                    "gl_Position = projection * view * modelPosition;\n"
                    "normal = normalize((model * vec4(vNormal, 0.0)).xyz);\n"
                    "position = modelPosition.xyz;\n"
                "}\n";
            const char fragShaderStringGL3[] =
                "#version 150\n"
                "out vec4 FragColor;\n"
                "uniform vec3 diffuseColor;\n"
                "uniform vec3 cameraPos;\n"
                "uniform mat4 view;\n"
                "in vec3 normal;\n"
                "in vec3 position;\n"
                "void main() {\n"
                    "vec3 lightPos = (vec4(-5.0, 0.0, -12.0, 1.0)).xyz;\n"
                    "vec3 lightDir = normalize(position - lightPos);\n"
                    "vec3 viewDir = normalize(position - cameraPos);\n "
                    "vec3 reflection = reflect(lightDir, normal);\n"
                    "float specular = clamp(dot(reflection, viewDir), 0.0, 1.0);\n"
                    "float intensity = clamp(dot(normal, lightDir), 0.0, 1.0);\n"
                    "FragColor = vec4(intensity * diffuseColor + pow(specular, 15) * vec3(1.0), 1.0);\n"
                "}\n";

            program =
                loadShaderProgram(NULL, drawAPI == ARG_API_GL3 ? vertShaderStringGL3 : vertShaderStringGLES2, NULL, drawAPI == ARG_API_GL3 ? fragShaderStringGL3 : fragShaderStringGLES2);

            glBindAttribLocation(program, ATTRIBUTE_VERTEX, "vPosition");
            glBindAttribLocation(program, ATTRIBUTE_NORMAL, "vNormal");

            // Retrieve linked uniform locations.
            uniforms[0][UNIFORM_VIEW_MATRIX] = glGetUniformLocation(program, "view");
            uniforms[0][UNIFORM_MODEL_MATRIX] = glGetUniformLocation(program, "model");
            uniforms[0][UNIFORM_PROJECTION_MATRIX] = glGetUniformLocation(program, "projection");
            uniforms[0][UNIFORM_DIFFUSE_COLOR] = glGetUniformLocation(program, "diffuseColor");
            uniforms[0][UNIFORM_CAMERA_POSITION] = glGetUniformLocation(program, "cameraPos");
        }

        if (!depthProgram) {
            const char vertShaderStringGLES2[] =
                "attribute vec3 position;\n"
                "uniform mat4 model;\n"
                "uniform mat4 view;\n"
                "uniform mat4 projection;\n"
                "out vec3 position;\n"
                "void main() {\n"
                    "vec4 modelPosition = view * model * vec4(vPosition, 1.0);\n"
                    "gl_Position = projection * modelPosition;\n"
                    "position = modelPosition.xyz;\n"
                "}\n";
            const char fragShaderStringGLES2[] =
                "#ifdef GL_ES\n"
                "precision mediump float;\n"
                "#endif\n"
                "in vec3 position;\n"
                "void main() {\n"
                    "gl_FragColor = vec4(position, 1.0);\n"
                "}\n";
            const char vertShaderStringGL3[] =
                "#version 150\n"
                "in vec3 vPosition;\n"
                "uniform mat4 model;\n"
                "uniform mat4 view;\n"
                "uniform mat4 projection;\n"
                "out vec3 position;\n"
                "void main() {\n"
                    "vec4 modelPosition = view * model * vec4(vPosition, 1.0);\n"
                    "gl_Position = projection * modelPosition;\n"
                    "position = modelPosition.xyz;\n"
                "}\n";
            const char fragShaderStringGL3[] =
                "#version 150\n"
                "out vec4 FragColor;\n"
                "in vec3 position;\n"
                "void main() {\n"
                    "FragColor = vec4(position, 1.0);\n"
                "}\n";

            depthProgram =
                loadShaderProgram(NULL, drawAPI == ARG_API_GL3 ? vertShaderStringGL3 : vertShaderStringGLES2, NULL, drawAPI == ARG_API_GL3 ? fragShaderStringGL3 : fragShaderStringGLES2);

            glBindAttribLocation(depthProgram, ATTRIBUTE_VERTEX, "vPosition");

            // Retrieve linked uniform locations.
            uniforms[1][UNIFORM_VIEW_MATRIX] = glGetUniformLocation(depthProgram, "view");
            uniforms[1][UNIFORM_MODEL_MATRIX] = glGetUniformLocation(depthProgram, "model");
            uniforms[1][UNIFORM_PROJECTION_MATRIX] = glGetUniformLocation(depthProgram, "projection");
        }
    }
#endif // HAVE_GLES2 || HAVE_GL3

    glEnable(GL_DEPTH_TEST);

    size_t poseIndex = 0;
    std::vector<float> modelDepth(contentWidth * contentHeight * 4);
    if (visible) {
        for (int i = 0; i < DRAW_MODELS_MAX; i++) {
            if (i == index && gModelLoaded[i] && gModelVisbilities[i]) {
                // Draw virtual model
                glUseProgram(program);
                drawModel(&(gModelPoses[i][0]), 0);

                // Draw view space coordinates of virtual model
                glUseProgram(depthProgram);
                glBindFramebuffer(GL_FRAMEBUFFER, gFBOs[2]);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                drawModel(&(gModelPoses[i][0]), 1);
                glReadPixels(0, 0, contentWidth, contentHeight, GL_RGBA, GL_FLOAT, modelDepth.data());

                poseIndex = i;
                break;
            }
        }
    }

    // Post processing step (stylization)
    drawPost(index, &(gModelPoses[poseIndex][0]), modelDepth, width, height, contentWidth, contentHeight);
}

static void drawModel(float pose[16], size_t uniform_index) {
    int i;
#if HAVE_GLES2 || HAVE_GL3
    float modelMatrix[16];
#endif

#if HAVE_GL
    if (drawAPI == ARG_API_GL) {
        glPushMatrix(); // Save world coordinate system.
        glScalef(0.05f, 0.05f, 0.05f);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        glTranslatef(0.0f, 1.25f, 0.0f);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        glVertexPointer(3, GL_FLOAT, 0, mVertices.data());
        glNormalPointer(GL_FLOAT, 0, mNormals.data());
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glColor3f(model.MeshMaterial.Kd.X, model.MeshMaterial.Kd.Y, model.MeshMaterial.Kd.Z);
        for (i = 0; i < model.Indices.size() / 3; i++) {
            glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, &model.Indices[i * 3]);
        }
        glPopMatrix();    // Restore world coordinate system.
    }
#endif // HAVE_GL

#if HAVE_GLES2 || HAVE_GL3
    if (drawAPI == ARG_API_GLES2 || drawAPI == ARG_API_GL3) {
        mtxLoadIdentityf(modelMatrix);
        mtxScalef(modelMatrix, 0.05f, 0.05f, 0.05f);
        mtxRotatef(modelMatrix, 90.0f, 1.0f, 0.0f, 0.0f);
        mtxTranslatef(modelMatrix, 0.0f, 1.25f, 0.0f);
        glUniformMatrix4fv(uniforms[uniform_index][UNIFORM_VIEW_MATRIX], 1, GL_FALSE, pose);
        glUniformMatrix4fv(uniforms[uniform_index][UNIFORM_MODEL_MATRIX], 1, GL_FALSE, modelMatrix);
        glUniformMatrix4fv(uniforms[uniform_index][UNIFORM_PROJECTION_MATRIX], 1, GL_FALSE, gProjection);
        glUniform3f(uniforms[uniform_index][UNIFORM_DIFFUSE_COLOR], model.MeshMaterial.Kd.X, model.MeshMaterial.Kd.Y, model.MeshMaterial.Kd.Z);
        float viewInv[16];
        invertMatrix(pose, viewInv);
        glUniform3f(uniforms[uniform_index][UNIFORM_CAMERA_POSITION], viewInv[12], viewInv[13], viewInv[14]);

#  if HAVE_GLES2
        if (drawAPI == ARG_API_GLES2) {
            glVertexAttribPointer(ATTRIBUTE_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, mVertices.data());
            glEnableVertexAttribArray(ATTRIBUTE_VERTEX);
            glVertexAttribPointer(ATTRIBUTE_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, mNormals.data());
            glEnableVertexAttribArray(ATTRIBUTE_NORMAL);
#    ifdef DEBUG
            if (!arglGLValidateProgram(program)) {
                ARLOGe("drawModel(): Error: shader program %d validation failed.\n", program);
                return;
            }
#    endif // DEBUG
            for (i = 0; i < model.Indices.size() / 3; i++) {
                glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, &model.Indices[i * 3]);
            }
        }
#  else // HAVE_GL3
        if (drawAPI == ARG_API_GL3) {
            if (!gModelVAO) {
                glGenVertexArrays(1, &gModelVAO);
                glBindVertexArray(gModelVAO);
                glGenBuffers(1, &gModelV3BO);
                glBindBuffer(GL_ARRAY_BUFFER, gModelV3BO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(objl::Vector3) * mVertices.size(), mVertices.data(), GL_STATIC_DRAW);
                glVertexAttribPointer(ATTRIBUTE_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, NULL);
                glEnableVertexAttribArray(ATTRIBUTE_VERTEX);
                glGenBuffers(1, &gModelN3BO);
                glBindBuffer(GL_ARRAY_BUFFER, gModelN3BO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(objl::Vector3) * mNormals.size(), mNormals.data(), GL_STATIC_DRAW);
                glVertexAttribPointer(ATTRIBUTE_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, NULL);
                glEnableVertexAttribArray(ATTRIBUTE_NORMAL);
                glGenBuffers(1, &gModelIBO);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gModelIBO);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * model.Indices.size(), model.Indices.data(), GL_STATIC_DRAW);
            }
            
            glBindVertexArray(gModelVAO);
    #ifdef DEBUG
            if (!arglGLValidateProgram(program)) {
                ARLOGe("drawModel() Error: shader program %d validation failed.\n", program);
                return;
            }
    #endif
            glDrawArrays(GL_TRIANGLES, 0, model.Indices.size());
        }
#  endif // HAVE_GL3
    }
#endif // HAVE_GLES2 || HAVE_GL3
#if HAVE_GL3
    glBindVertexArray(0);
#endif
}

static void drawPost(size_t index, const float pose[16], const std::vector<float>& modelDepth, int width, int height, int contentWidth, int contentHeight) {
    const GLfloat vertices [6][2] = {
        {-1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f},
        {-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f} };
    const GLfloat texCoords [6][2] = {
        {0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f} };

#if HAVE_GLES2 || HAVE_GL3
    if (drawAPI == ARG_API_GLES2 || drawAPI == ARG_API_GL3) {
        if (!postProgram) {
            const char *resourcesDir = arUtilGetResourcesDirectoryPath(
                AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST);
            const std::string voronoiString = std::string(resourcesDir) + "/voronoi.frag";
            const std::string fragPathGLES2 = std::string(resourcesDir) + "/shaderES2.frag";

            const char vertShaderStringGLES2[] =
                "attribute vec2 position;\n"
                "attribute vec2 texCoord;\n"
                "out vec2 vTexCoord;\n"
                "void main() {\n"
                    "gl_Position = vec4(position.x, position.y, 0.0, 1.0);\n"
                    "vTexCoord = texCoord;\n"
                "}\n";
            const char vertShaderStringGL3[] =
                "#version 150\n"
                "in vec2 position;\n"
                "in vec2 texCoord;\n"
                "out vec2 vTexCoord;\n"
                "void main() {\n"
                    "gl_Position = vec4(position.x, position.y, 0.0, 1.0);\n"
                    "vTexCoord = texCoord;\n"
                "}\n";

            postProgram =
                loadShaderProgram(NULL, drawAPI == ARG_API_GL3 ? vertShaderStringGL3 : vertShaderStringGLES2, drawAPI == ARG_API_GL3 ? voronoiString.c_str() : fragPathGLES2.c_str(), NULL);

            glBindAttribLocation(postProgram, ATTRIBUTE_VERTEX, "position");
            glBindAttribLocation(postProgram, ATTRIBUTE_NORMAL, "texCoord");

            uniforms[0][UNIFORM_WIDTH] = glGetUniformLocation(postProgram, "width");
            uniforms[0][UNIFORM_HEIGHT] = glGetUniformLocation(postProgram, "height");
        }
    }
#  if HAVE_GLES2
        if (drawAPI == ARG_API_GLES2) {
            glVertexAttribPointer(ATTRIBUTE_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, vertices);
            glVertexAttribPointer(ATTRIBUTE_NORMAL, 2, GL_FLOAT, GL_FALSE, 0, texCoords);
            glEnableVertexAttribArray(ATTRIBUTE_VERTEX);
            glEnableVertexAttribArray(ATTRIBUTE_NORMAL);
#    ifdef DEBUG
            if (!arglGLValidateProgram(postProgram)) {
                ARLOGe("drawPost(): Error: shader program %d validation failed.\n", postProgram);
                return;
            }
#    endif // DEBUG
        }
#  else // HAVE_GL3
        if (drawAPI == ARG_API_GL3) {
            if (!gQuadVAO) {
                glGenVertexArrays(1, &gQuadVAO);
                glBindVertexArray(gQuadVAO);
                glGenBuffers(2, gQuadVBOs);
                glBindBuffer(GL_ARRAY_BUFFER, gQuadVBOs[0]);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                glVertexAttribPointer(ATTRIBUTE_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, NULL);
                glEnableVertexAttribArray(ATTRIBUTE_VERTEX);
                glBindBuffer(GL_ARRAY_BUFFER, gQuadVBOs[1]);
                glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);
                glVertexAttribPointer(ATTRIBUTE_NORMAL, 2, GL_FLOAT, GL_FALSE, 0, NULL);
                glEnableVertexAttribArray(ATTRIBUTE_NORMAL);
            }
    #ifdef DEBUG
            if (!arglGLValidateProgram(postProgram)) {
                ARLOGe("drawPost() Error: shader program %d validation failed.\n", postProgram);
                return;
            }
    #endif
        }
#  endif

    const cv::Mat& depth = index == 0 ? leftDepth : rightDepth;
#  if !BASELINE // This boolean determines whether to run anchor repositioning and density redistribution
    voronoi[index]->updateDepth(depth, modelDepth, pose, gProjection, width, height, contentWidth, contentHeight);

    glBindFramebuffer(GL_FRAMEBUFFER, gFBOs[1]);
    glBindTexture(GL_TEXTURE_2D, gFBOTextures[0]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    voronoi[index]->drawPattern(true);

#   if HAVE_GL3
    glBindVertexArray(gQuadVAO);
    glUseProgram(postProgram);
#   endif

    glUniform1f(uniforms[0][UNIFORM_WIDTH], float(contentWidth));
    glUniform1f(uniforms[0][UNIFORM_HEIGHT], float(contentHeight));
    glBindTexture(GL_TEXTURE_2D, gFBOTextures[1]);
    voronoi[index]->updateDensity(postProgram, depth, modelDepth, pose, width, height, contentWidth, contentHeight);
#  endif

    glViewport(gViewport[0] + gViewport[2] / 2 * index, gViewport[1], gViewport[2] / 2, gViewport[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, gFBOTextures[0]);

    voronoi[index]->drawPattern(false);

# if false // This boolean determines whether to run the quantitative measurement or not
    std::vector<unsigned char> pixels(contentWidth * contentHeight * 4);
    glReadPixels(gViewport[0] + gViewport[2] / 2 * index, gViewport[1], contentWidth, contentHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // Compare pixels with the same world positions from last frame
    float viewInv[16];
    invertMatrix(pose, viewInv);
    if (!worldPositions[index].empty()) {
        // Change 0 to 1 for visual coherence
        const size_t newIndex = (index + 1) % 2;
        std::vector<float> differencesRaw;
        differencesRaw.reserve(contentWidth * contentHeight);
        float count = 0.0f;

        for (size_t i = 0; i < contentHeight; i++) {
            for (size_t j = 0; j < contentWidth; j++) {
                const size_t index2 = i * contentWidth + j;
                const float worldPos[4] = {
                    worldPositions[newIndex][index2][0],
                    worldPositions[newIndex][index2][1],
                    worldPositions[newIndex][index2][2],
                    1.0f,
                };
                float viewPos[4];
                transformVector(worldPos, pose, viewPos);

                for (int j = 0; j < 4; j++) {
                    viewPos[j] /= viewPos[3];
                }

                float imagePos[4];
                transformVector(viewPos, gProjection, imagePos);

                for (int j = 0; j < 4; j++) {
                    imagePos[j] /= imagePos[3];
                }

                const float x = (imagePos[0] + 1.0f) / 2.0f;
                const float y = 1.0f - (imagePos[1] + 1.0f) / 2.0f;
                if (isnan(x) || isnan(y) || isinf(x) || isinf(y) || x < 0.0f || y < 0.0f || x >= 1.0f || y >= 1.0f) {
                    differencesRaw.push_back(-1.0f);
                    continue;
                }

                const size_t modelIndex = ((contentHeight - int(y * float(contentHeight)) - 1) * contentWidth + int(x * float(contentHeight))) * 4;
                cv::Vec3f v = cv::Vec3f(0.0f, 0.0f, 0.0f);

                if (modelIndex < modelDepth.size() - 2) {
                    v = cv::Vec3f(
                        modelDepth[modelIndex + 0],
                        modelDepth[modelIndex + 1],
                        modelDepth[modelIndex + 2]
                    );
                }

                if (v[0] == 0.0f && v[1] == 0.0f && v[2] == 0.0f) {
                    const size_t ix = int(x * float(width));
                    const size_t iy = int(y * float(height));
                    v = depth.at<cv::Vec3f>(iy, ix);
                }

                if (cv::norm(v, cv::Vec3f(viewPos), cv::NORM_L2) <= 0.1f) {
                    const size_t ix = int(x * float(contentWidth));
                    const size_t iy = contentHeight - int(y * float(contentHeight)) - 1;
                    const cv::Vec3b color = colors[newIndex][index2];
                    const size_t pixelIndex = (iy * contentWidth + ix) * 4;
                    const float value = cv::norm(cv::Vec3f(color) / 255.0, cv::Vec3f(pixels[pixelIndex + 0], pixels[pixelIndex + 1], pixels[pixelIndex + 2]) / 255.0, cv::NORM_L2);
                    differencesRaw.push_back(value);
                    count += value;
                } else {
                    differencesRaw.push_back(-1.0f);
                }
            }
        }

#  if true // Constant normalization factor for visualization
        float normFactor = 0.1f;
#  else // Non-constant normalization factor based on maximum of 3 standard deviations from the mean
        const float mean = count / float(contentWidth * contentHeight);
        float sd = 0.0f;
        for (float x : differencesRaw) {
            if (x == -1.0f) {
                continue;
            }
            sd += (x - mean) * (x - mean);
        }

        sd /= float(contentWidth * contentHeight - 1);

        for (float x : differencesRaw) {
            if (x == -1.0f || x - mean > 3 * sd) {
                continue;
            }
            normFactor = std::max(normFactor, x);
        }
#  endif

        printf("Index %d: %f\n", index, count);
        std::vector<cv::Vec3b> differences;
        differences.reserve(contentWidth * contentHeight);
        for (size_t i = 0; i < contentHeight; i++) {
            for (size_t j = 0; j < contentWidth; j++) {
                const float diff = differencesRaw[(contentHeight - i - 1) * contentWidth + j];
                if (diff == -1.0f) {
                    differences.push_back(cv::Vec3b(0, 255, 0));
                } else {
                    differences.push_back(cv::Vec3b(0, 0, int(diff / normFactor * 256.0f)));
                }
            }
        }

        // Save pixel difference images
        // cv::Mat diffImage(contentHeight, contentWidth, CV_8UC3, differences.data());
        // if (index == 0) {
        //     cv::imwrite("differenceL.jpg", diffImage);
        // } else {
        //     cv::imwrite("differenceR.jpg", diffImage);
        // }
    }

    // Save pixel world positions for current frame
    worldPositions[index].resize(contentWidth * contentHeight);
    colors[index].resize(contentWidth * contentHeight);
    for (size_t i = 0; i < contentHeight; i++) {
        for (size_t j = 0; j < contentWidth; j++) {
            const size_t index2 = i * contentWidth + j;
            const float x = float(j) / float(contentWidth);
            const float y = float(contentHeight - i - 1) / float(contentHeight);

            cv::Vec3f v = cv::Vec3f(0.0f, 0.0f, 0.0f);
            if (index2 * 4 < modelDepth.size() - 2) {
                v = cv::Vec3f(
                    modelDepth[index2 * 4 + 0],
                    modelDepth[index2 * 4 + 1],
                    modelDepth[index2 * 4 + 2]
                );
            }

            if (v[0] == 0.0f && v[1] == 0.0f && v[2] == 0.0f) {
                v = depth.at<cv::Vec3f>(int(y * float(height)), int(x * float(width)));
            }

            const float P[4] = {
                v[0],
                v[1],
                v[2],
                1.0f
            };
            float worldPos[4];
            transformVector(P, viewInv, worldPos);
            colors[index][index2] = cv::Vec3b(pixels[index2 * 4 + 0], pixels[index2 * 4 + 1], pixels[index2 * 4 + 2]);
            worldPositions[index][index2] = cv::Vec3f(worldPos[0], worldPos[1], worldPos[2]);
        }
    }
# endif

    glBindTexture(GL_TEXTURE_2D, 0);
#endif
#if HAVE_GL3
    glBindVertexArray(0);
#endif
}
