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
#define PROGRAM_COUNT 5

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
static GLint uniforms[UNIFORM_COUNT] = {0};
static GLuint program = 0;
static GLuint postPrograms[PROGRAM_COUNT] = {0};

#if HAVE_GL3
static GLuint gModelVAO = 0;
static GLuint gModelV3BO = 0;
static GLuint gModelN3BO = 0;
static GLuint gModelIBO = 0;
static GLuint gQuadVAO = 0;
static GLuint gQuadVBOs[2] = {0};

static GLuint gFBOs[2] = {0};
static GLuint gFBOTextures[2] = {0};
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
static bool stylize = true;

static int32_t gViewport[4] = {0};
static float gProjection[16];
static float gView[16];
static bool gModelLoaded[DRAW_MODELS_MAX] = {false};
static float gModelPoses[DRAW_MODELS_MAX][16];
static bool gModelVisbilities[DRAW_MODELS_MAX];

static objl::Mesh model;
static std::vector<float> mVertices;
static std::vector<float> mNormals;
static Voronoi* voronoi;

static void drawModel(float pose[16]);
static void drawPost(size_t index);

void drawInit() {
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
}

void drawToggleModels() {
    visible = !visible;
}

void drawToggleStyle() {
    stylize = !stylize;
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

    voronoi = new Voronoi(drawAPI, 80, 80, width, height);

#if HAVE_GLES2 || HAVE_GL3
    glActiveTexture(GL_TEXTURE0);
    // Create framebuffers
    glGenFramebuffers(2, gFBOs);

    // Create framebuffer textures
    glGenTextures(2, gFBOTextures);

    // Create render buffers
    glGenRenderbuffers(2, gRBOs);

    for (int j = 0; j <= 1; j++) {
        glBindTexture(GL_TEXTURE_2D, gFBOTextures[j]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindRenderbuffer(GL_RENDERBUFFER, gRBOs[j]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

        glBindFramebuffer(GL_FRAMEBUFFER, gFBOs[j]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gFBOTextures[j], 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gRBOs[j]);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif

    return;
}

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
    delete voronoi;

    return;
}

int drawLoadModel(const char *path) {
    // Ignore path, we'll always draw a cube.
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

void draw(size_t index) {
    glViewport(0, 0, gViewport[2] / 2, gViewport[3]);

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
                    "vec3 lightPos = (vec4(-50.0, 0.0, -80.0, 1.0)).xyz;\n"
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
                    "vec3 lightPos = (vec4(-50.0, 0.0, -80.0, 1.0)).xyz;\n"
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
            uniforms[UNIFORM_VIEW_MATRIX] = glGetUniformLocation(program, "view");
            uniforms[UNIFORM_MODEL_MATRIX] = glGetUniformLocation(program, "model");
            uniforms[UNIFORM_PROJECTION_MATRIX] = glGetUniformLocation(program, "projection");
            uniforms[UNIFORM_DIFFUSE_COLOR] = glGetUniformLocation(program, "diffuseColor");
            uniforms[UNIFORM_CAMERA_POSITION] = glGetUniformLocation(program, "cameraPos");
        }
        glUseProgram(program);
    }
#endif // HAVE_GLES2 || HAVE_GL3

    glEnable(GL_DEPTH_TEST);

    if (visible) {
        for (int i = 0; i < DRAW_MODELS_MAX; i++) {
            if (i == index && gModelLoaded[i] && gModelVisbilities[i]) {
                drawModel(&(gModelPoses[i][0]));
            }
        }
    }

    drawPost(index);
}

// Something to look at, draw a rotating colour cube.
static void drawModel(float pose[16]) {
    int i;
#if HAVE_GLES2 || HAVE_GL3
    float modelMatrix[16];
#endif

#if HAVE_GL
    if (drawAPI == ARG_API_GL) {
        glPushMatrix(); // Save world coordinate system.
        glScalef(40.0f, 40.0f, 40.0f);
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
        mtxScalef(modelMatrix, 40.0f, 40.0f, 40.0f);
        mtxRotatef(modelMatrix, 90.0f, 1.0f, 0.0f, 0.0f);
        mtxTranslatef(modelMatrix, 0.0f, 1.25f, 0.0f);
        glUniformMatrix4fv(uniforms[UNIFORM_VIEW_MATRIX], 1, GL_FALSE, pose);
        glUniformMatrix4fv(uniforms[UNIFORM_MODEL_MATRIX], 1, GL_FALSE, modelMatrix);
        glUniformMatrix4fv(uniforms[UNIFORM_PROJECTION_MATRIX], 1, GL_FALSE, gProjection);
        glUniform3f(uniforms[UNIFORM_DIFFUSE_COLOR], model.MeshMaterial.Kd.X, model.MeshMaterial.Kd.Y, model.MeshMaterial.Kd.Z);
        float viewInv[16];
        invertMatrix(pose, viewInv);
        glUniform3f(uniforms[UNIFORM_CAMERA_POSITION], viewInv[12], viewInv[13], viewInv[14]);
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

static void drawPost(size_t index) {
    const GLfloat vertices [6][2] = {
        {-1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f},
        {-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f} };
    const GLfloat texCoords [6][2] = {
        {0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f} };

    glViewport(0, 0, gViewport[2] / 2, gViewport[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, gFBOs[1]);

    voronoi->drawPattern();

#if HAVE_GLES2 || HAVE_GL3
    if (drawAPI == ARG_API_GLES2 || drawAPI == ARG_API_GL3) {
        if (!postPrograms[0]) {
            const char *resourcesDir = arUtilGetResourcesDirectoryPath(
                AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST);
            const std::string rgb2lab = std::string(resourcesDir) + "/rgb2lab.frag";
            const std::string filter = std::string(resourcesDir) + "/filter.frag";
            const std::string edge = std::string(resourcesDir) + "/edge.frag";
            const std::string lab2rgb = std::string(resourcesDir) + "/lab2rgb.frag";
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

            postPrograms[0] =
                loadShaderProgram(NULL, drawAPI == ARG_API_GL3 ? vertShaderStringGL3 : vertShaderStringGLES2, drawAPI == ARG_API_GL3 ? rgb2lab.c_str() : fragPathGLES2.c_str(), NULL);
            postPrograms[1] =
                loadShaderProgram(NULL, drawAPI == ARG_API_GL3 ? vertShaderStringGL3 : vertShaderStringGLES2, drawAPI == ARG_API_GL3 ? filter.c_str() : fragPathGLES2.c_str(), NULL);
            postPrograms[2] =
                loadShaderProgram(NULL, drawAPI == ARG_API_GL3 ? vertShaderStringGL3 : vertShaderStringGLES2, drawAPI == ARG_API_GL3 ? edge.c_str() : fragPathGLES2.c_str(), NULL);
            postPrograms[3] =
                loadShaderProgram(NULL, drawAPI == ARG_API_GL3 ? vertShaderStringGL3 : vertShaderStringGLES2, drawAPI == ARG_API_GL3 ? lab2rgb.c_str() : fragPathGLES2.c_str(), NULL);
            postPrograms[4] =
                loadShaderProgram(NULL, drawAPI == ARG_API_GL3 ? vertShaderStringGL3 : vertShaderStringGLES2, drawAPI == ARG_API_GL3 ? voronoiString.c_str() : fragPathGLES2.c_str(), NULL);

            for (size_t i = 0; i < PROGRAM_COUNT; i++) {
                glBindAttribLocation(postPrograms[i], ATTRIBUTE_VERTEX, "position");
                glBindAttribLocation(postPrograms[i], ATTRIBUTE_NORMAL, "texCoord");
            }

            voronoi->load(postPrograms[4]);

            uniforms[UNIFORM_WIDTH] = glGetUniformLocation(postPrograms[2], "width");
            uniforms[UNIFORM_HEIGHT] = glGetUniformLocation(postPrograms[2], "height");
        }
    }
#  if HAVE_GLES2
        if (drawAPI == ARG_API_GLES2) {
            glVertexAttribPointer(ATTRIBUTE_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, vertices);
            glVertexAttribPointer(ATTRIBUTE_NORMAL, 2, GL_FLOAT, GL_FALSE, 0, texCoords);
            glEnableVertexAttribArray(ATTRIBUTE_VERTEX);
            glEnableVertexAttribArray(ATTRIBUTE_NORMAL);
#    ifdef DEBUG
            if (!arglGLValidateProgram(postPrograms)) {
                ARLOGe("drawPost(): Error: shader program %d validation failed.\n", postPrograms);
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

            glBindVertexArray(gQuadVAO);
    #ifdef DEBUG
            if (!arglGLValidateProgram(postPrograms)) {
                ARLOGe("drawPost() Error: shader program %d validation failed.\n", postPrograms);
                return;
            }
    #endif
        }
#  endif
    glViewport(gViewport[0] + gViewport[2] / 2 * index, gViewport[1], gViewport[2] / 2, gViewport[3]);
    glUseProgram(postPrograms[4]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gFBOTextures[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gFBOTextures[1]);

    voronoi->prepare();

    glDrawArrays(GL_TRIANGLES, 0, 6);

    // // RGB to LAB
    // glUseProgram(postPrograms[0]);
    // glBindFramebuffer(GL_FRAMEBUFFER, gFBOs[1]);
    // glClear(GL_COLOR_BUFFER_BIT);
    // glBindTexture(GL_TEXTURE_2D, gFBOTextures[0]);

    // glDrawArrays(GL_TRIANGLES, 0, 6);

    // if (stylize) {
    //     // Bilinear filter
    //     glUseProgram(postPrograms[1]);
    //     glBindFramebuffer(GL_FRAMEBUFFER, gFBOs[0]);
    //     glClear(GL_COLOR_BUFFER_BIT);
    //     glBindTexture(GL_TEXTURE_2D, gFBOTextures[1]);

    //     glUniform1f(uniforms[UNIFORM_WIDTH], float(gViewport[2] / 2));
    //     glUniform1f(uniforms[UNIFORM_HEIGHT], float(gViewport[3]));

    //     glDrawArrays(GL_TRIANGLES, 0, 6);

    //     // Edge detection
    //     glUseProgram(postPrograms[2]);
    //     glBindFramebuffer(GL_FRAMEBUFFER, gFBOs[1]);
    //     glClear(GL_COLOR_BUFFER_BIT);
    //     glBindTexture(GL_TEXTURE_2D, gFBOTextures[0]);

    //     glUniform1f(uniforms[UNIFORM_WIDTH], float(gViewport[2] / 2));
    //     glUniform1f(uniforms[UNIFORM_HEIGHT], float(gViewport[3]));

    //     glDrawArrays(GL_TRIANGLES, 0, 6);
    // }

    // // LAB to RGB
    // glViewport(gViewport[0] + gViewport[2] / 2 * index, gViewport[1], gViewport[2] / 2, gViewport[3]);

    // glUseProgram(postPrograms[3]);
    // glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // glBindTexture(GL_TEXTURE_2D, gFBOTextures[1]);

    // glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindTexture(GL_TEXTURE_2D, 0);
#endif
#if HAVE_GL3
    glBindVertexArray(0);
#endif
}
