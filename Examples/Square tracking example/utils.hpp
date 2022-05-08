#pragma once

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

// MESA glu invert matrix
static bool invertMatrix(const float m[16], float invOut[16]) {
    float inv[16], det;
    int i;

    inv[0] = m[5]  * m[10] * m[15] -
             m[5]  * m[11] * m[14] -
             m[9]  * m[6]  * m[15] +
             m[9]  * m[7]  * m[14] +
             m[13] * m[6]  * m[11] -
             m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] +
              m[4]  * m[11] * m[14] +
              m[8]  * m[6]  * m[15] -
              m[8]  * m[7]  * m[14] -
              m[12] * m[6]  * m[11] +
              m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] -
             m[4]  * m[11] * m[13] -
             m[8]  * m[5] * m[15] +
             m[8]  * m[7] * m[13] +
             m[12] * m[5] * m[11] -
             m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] +
               m[4]  * m[10] * m[13] +
               m[8]  * m[5] * m[14] -
               m[8]  * m[6] * m[13] -
               m[12] * m[5] * m[10] +
               m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] +
              m[1]  * m[11] * m[14] +
              m[9]  * m[2] * m[15] -
              m[9]  * m[3] * m[14] -
              m[13] * m[2] * m[11] +
              m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] -
             m[0]  * m[11] * m[14] -
             m[8]  * m[2] * m[15] +
             m[8]  * m[3] * m[14] +
             m[12] * m[2] * m[11] -
             m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] +
              m[0]  * m[11] * m[13] +
              m[8]  * m[1] * m[15] -
              m[8]  * m[3] * m[13] -
              m[12] * m[1] * m[11] +
              m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] -
              m[0]  * m[10] * m[13] -
              m[8]  * m[1] * m[14] +
              m[8]  * m[2] * m[13] +
              m[12] * m[1] * m[10] -
              m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] -
             m[1]  * m[7] * m[14] -
             m[5]  * m[2] * m[15] +
             m[5]  * m[3] * m[14] +
             m[13] * m[2] * m[7] -
             m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] +
              m[0]  * m[7] * m[14] +
              m[4]  * m[2] * m[15] -
              m[4]  * m[3] * m[14] -
              m[12] * m[2] * m[7] +
              m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] -
              m[0]  * m[7] * m[13] -
              m[4]  * m[1] * m[15] +
              m[4]  * m[3] * m[13] +
              m[12] * m[1] * m[7] -
              m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] +
               m[0]  * m[6] * m[13] +
               m[4]  * m[1] * m[14] -
               m[4]  * m[2] * m[13] -
               m[12] * m[1] * m[6] +
               m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] +
              m[1] * m[7] * m[10] +
              m[5] * m[2] * m[11] -
              m[5] * m[3] * m[10] -
              m[9] * m[2] * m[7] +
              m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] -
             m[0] * m[7] * m[10] -
             m[4] * m[2] * m[11] +
             m[4] * m[3] * m[10] +
             m[8] * m[2] * m[7] -
             m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] +
               m[0] * m[7] * m[9] +
               m[4] * m[1] * m[11] -
               m[4] * m[3] * m[9] -
               m[8] * m[1] * m[7] +
               m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] -
              m[0] * m[6] * m[9] -
              m[4] * m[1] * m[10] +
              m[4] * m[2] * m[9] +
              m[8] * m[1] * m[6] -
              m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det == 0)
        return false;

    det = 1.0f / det;

    for (i = 0; i < 16; i++)
        invOut[i] = inv[i] * det;

    return true;
}

static GLuint loadShader(const char* shaderPath, const char* shaderString, GLenum type) {
    GLuint shader;
    GLint status = 0;
    if (shaderPath) {
        status = arglGLCompileShaderFromFile(&shader, type, shaderPath);
    } else {
        status = arglGLCompileShaderFromString(&shader, type, shaderString);
    }
    if (!status) {
        ARLOGe("loadShaderProgram: Error compiling shader.\n");
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        char errorLog[maxLength];
        glGetShaderInfoLog(shader, maxLength, &maxLength, errorLog);

        printf("%s\n", errorLog);
        return 0;
    }
    return shader;
}

static GLuint loadShaderProgram(const char* vertPath, const char* vertString, const char* fragPath, const char* fragString) {
    GLuint program = glCreateProgram();
    GLuint vertShader, fragShader;
    if (!program) {
        ARLOGe("loadShaderProgram: Error creating shader program.\n");
        return 0;
    }
    vertShader = loadShader(vertPath, vertString, GL_VERTEX_SHADER);
    fragShader = loadShader(fragPath, fragString, GL_FRAGMENT_SHADER);
    if (!vertShader || !fragShader) {
        arglGLDestroyShaders(vertShader, fragShader, program);
        return 0;
    }

    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);

    if (!arglGLLinkProgram(program)) {
        ARLOGe("loadShaderProgram: Error linking shader program.\n");
        arglGLDestroyShaders(vertShader, fragShader, program);
        return 0;
    }

    arglGLDestroyShaders(vertShader, fragShader, 0);

    return program;
}
