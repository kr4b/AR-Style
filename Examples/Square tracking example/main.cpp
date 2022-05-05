//
//  main.cpp
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef DEBUG
#ifdef _WIN32
#define MAXPATHLEN MAX_PATH
#include <direct.h> // _getcwd()
#define getcwd _getcwd
#else
#include <sys/param.h>
#include <unistd.h>
#endif
#endif
#include <string>

#include <ARX/ARController.h>
#include <ARX/ARUtil/time.h>

#ifdef __APPLE__
#include <SDL2/SDL.h>
#if (HAVE_GL || HAVE_GL3)
#include <SDL2/SDL_opengl.h>
#elif HAVE_GLES2
#include <SDL2/SDL_opengles2.h>
#endif
#else
#include "SDL2/SDL.h"
#if (HAVE_GL || HAVE_GL3)
#include "SDL2/SDL_opengl.h"
#elif HAVE_GLES2
#include "SDL2/SDL_opengles2.h"
#endif
#endif

#include "draw.h"

#if ARX_TARGET_PLATFORM_WINDOWS
const char *vconfl = "-module=WinMF -format=BGRA";
const char *vconfr = "-module=WinMF -format=BGRA";
#else
// const char *vconf = "-module=GStreamer filesrc location=/home/max/test.mp4 !"
//                     "decodebin ! videoconvert ! video/x-raw ! identity "
//                     "name=artoolkit ! fakesink";
// const char *vconf =
//     "-module=V4L2 -width=1280 -height=720 -dev=/dev/video4 -format=BGRA";
const char *vconfl = NULL;
const char *vconfr = NULL;
#endif
const char *cpara = NULL;

// Window and GL context.
static SDL_GLContext gSDLContext = NULL;
static int contextWidth = 0;
static int contextHeight = 0;
static bool contextWasUpdated = false;
static int32_t viewport[4];
static float projection[16];
static SDL_Window *gSDLWindow = NULL;

static ARController* arControllers[2] = {NULL};
static ARG_API drawAPI = ARG_API_None;

static long gFrameNo = 0;

struct marker {
  const char *name;
  float height;
};
static const struct marker markers[] = { //{"hiro.patt", 80.0},
    {"kanji.patt", 80.0}};
static const int markerCount = (sizeof(markers) / sizeof(markers[0]));

//
//
//

static void processCommandLineOptions(int argc, char *argv[]);
static void usage(char *com);
static void quit(int rc);
static void reshape(int w, int h);

int main(int argc, char *argv[]) {
  processCommandLineOptions(argc, argv);

  // Initialize SDL.
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    ARLOGe("Error: SDL initialisation failed. SDL error: '%s'.\n",
           SDL_GetError());
    quit(1);
    return 1;
  }

  // Create a window.
#if 0
    gSDLWindow = SDL_CreateWindow("artoolkitX Square Tracking Example",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              0, 0,
                              SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
                              );
#else
  gSDLWindow = SDL_CreateWindow(
      "artoolkitX Square Tracking Example", SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED, 1280, 720,
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
#endif
  if (!gSDLWindow) {
    ARLOGe("Error creating window: %s.\n", SDL_GetError());
    quit(-1);
  }

  // Create an OpenGL context to draw into. If OpenGL 3.2 not available, attempt
  // to fall back to OpenGL 1.5, then OpenGL ES 2.0
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); // This is the default.
  SDL_GL_SetSwapInterval(1);
#if HAVE_GL3
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  gSDLContext = SDL_GL_CreateContext(gSDLWindow);
  if (gSDLContext) {
    drawAPI = ARG_API_GL3;
    ARLOGi("Created OpenGL 3.2+ context.\n");
  } else {
    ARLOGi("Unable to create OpenGL 3.2 context: %s. Will try OpenGL 1.5.\n",
           SDL_GetError());
#endif // HAVE_GL3
#if HAVE_GL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0);
    gSDLContext = SDL_GL_CreateContext(gSDLWindow);
    if (gSDLContext) {
      drawAPI = ARG_API_GL;
      ARLOGi("Created OpenGL 1.5+ context.\n");
#if ARX_TARGET_PLATFORM_MACOS
      vconfl = "-format=BGRA";
      vconfr = "-format=BGRA";
#endif
    } else {
      ARLOGi(
          "Unable to create OpenGL 1.5 context: %s. Will try OpenGL ES 2.0\n",
          SDL_GetError());
#endif // HAVE_GL
#if HAVE_GLES2
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                          SDL_GL_CONTEXT_PROFILE_ES);
      gSDLContext = SDL_GL_CreateContext(gSDLWindow);
      if (gSDLContext) {
        drawAPI = ARG_API_GLES2;
        ARLOGi("Created OpenGL ES 2.0+ context.\n");
      } else {
        ARLOGi("Unable to create OpenGL ES 2.0 context: %s.\n", SDL_GetError());
      }
#endif // HAVE_GLES2
#if HAVE_GL
    }
#endif
#if HAVE_GL3
  }
#endif
  if (drawAPI == ARG_API_None) {
    ARLOGe("No OpenGL context available. Giving up.\n", SDL_GetError());
    quit(-1);
  }

  int w, h;
  SDL_GL_GetDrawableSize(SDL_GL_GetCurrentWindow(), &w, &h);
  reshape(w, h);

  // Initialise the ARControllers
  for (int i = 0; i <= 1; i++) {
    arControllers[i] = new ARController();
    if (!arControllers[i]->initialiseBase()) {
      ARLOGe("Error initialising ARController.\n");
      quit(-1);
    }
  }

#ifdef DEBUG
  arLogLevel = AR_LOG_LEVEL_DEBUG;
#endif

  // Add trackables.
  int markerIDs[2][markerCount];
  int markerModelIDs[2][markerCount];
#ifdef DEBUG
  char buf[MAXPATHLEN];
  ARLOGd("CWD is '%s'.\n", getcwd(buf, sizeof(buf)));
#endif
  char *resourcesDir = arUtilGetResourcesDirectoryPath(
      AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST);
  for (int i = 0; i<= 1; i++) {
    for (int j = 0; j < markerCount; j++) {
      std::string markerConfig = "single;" + std::string(resourcesDir) + '/' +
                                markers[j].name + ';' +
                                std::to_string(markers[j].height);
      markerIDs[i][j] = arControllers[i]->addTrackable(markerConfig);
      if (markerIDs[i][j] == -1) {
        ARLOGe("Error adding marker.\n");
        quit(-1);
      }
    }
    arControllers[i]->getSquareTracker()->setPatternDetectionMode(
        AR_TEMPLATE_MATCHING_MONO);
    arControllers[i]->getSquareTracker()->setThresholdMode(
        AR_LABELING_THRESH_MODE_AUTO_BRACKETING);
  }
#ifdef DEBUG
  ARLOGd("vconfl is '%s'.\n", vconfl);
  ARLOGd("vconfr is '%s'.\n", vconfr);
#endif
  // Stereo projection matrix
  // const double stereoParametersRaw[12] = {
  //   0.99961674, -0.02704036, -0.00593392,  0.36404321,
  //   -0.01923079, -0.52406532, -0.85146105, -0.03396377,
  //   0.01991405,  0.85124886, -0.52438444,  0.34131616
  // };
  const double stereoParametersRaw[12] = {
    1.0, 0.0, 0.0, 0.095,
    0.0, 1.0, 0.0, 0.095,
    0.0, 0.0, 1.0, 0.095
  };
  // const double stereoParametersRaw[12] = {
  //   1.0, 0.0, 0.0, 0.0,
  //   0.0, 1.0, 0.0, 0.0,
  //   0.0, 0.0, 1.0, 0.0,
  // };
  char stereoParameters[sizeof(stereoParametersRaw)];
  for (int i = 0; i < 12; i++) {
    for (int j = 0; j < sizeof(double); j++) {
      stereoParameters[i * sizeof(double) + sizeof(double) - j - 1] = ((char*) &stereoParametersRaw[i])[j];
    }
  }

  // Start tracking.
  // arController->startRunningStereo(vconfl, cpara, NULL, 0, vconfr, cpara, NULL, 0, NULL, (const char*) stereoParameters, sizeof(stereoParameters));
  arControllers[0]->startRunning(vconfl, cpara, NULL, 0);
  arControllers[1]->startRunning(vconfr, cpara, NULL, 0);

  drawInit();

  bool paused = true;
  bool firstFrame = true;

  // Main loop.
  bool done = false;
  while (!done) {

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_QUIT ||
          (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)) {
        done = true;
        break;
      } else if (ev.type == SDL_WINDOWEVENT) {
        // ARLOGd("Window event %d.\n", ev.window.event);
        if (ev.window.event == SDL_WINDOWEVENT_RESIZED &&
            ev.window.windowID == SDL_GetWindowID(gSDLWindow)) {
          // int32_t w = ev.window.data1;
          // int32_t h = ev.window.data2;
          int w, h;
          SDL_GL_GetDrawableSize(gSDLWindow, &w, &h);
          reshape(w, h);
        }
      } else if (ev.type == SDL_KEYDOWN) {
        if (ev.key.keysym.sym == SDLK_d) {
          for (int i = 0; i <= 1; i++) {
            arControllers[i]->getSquareTracker()->setDebugMode(
                !arControllers[i]->getSquareTracker()->debugMode());
          }
        }
        if (ev.key.keysym.sym == SDLK_v) {
          drawToggleModels();
        }
        if (ev.key.keysym.sym == SDLK_TAB) {
          drawToggleStyle();
        }
        if (ev.key.keysym.sym == SDLK_SPACE) {
          paused = !paused;
        }
      }
    }

    bool gotFrame = true;
    if (paused && !firstFrame) {
      gotFrame = false;
    } else {
      firstFrame = false;
      for (int i = 0; i <= 1; i++) {
        gotFrame &= arControllers[i]->capture();
      }
    }

    if (!gotFrame) {
      arUtilSleep(1);
    } else {
      // ARLOGi("Got frame %ld.\n", gFrameNo);
      gFrameNo++;

      for (int i = 0; i <= 1; i++) {
        if (!arControllers[i]->update()) {
          ARLOGe("Error in ARController::update().\n");
          quit(-1);
        }
      }

      if (contextWasUpdated) {

        for (int i = 0; i <= 1; i++) {
          if (!arControllers[i]->drawVideoInit(0)) {
            ARLOGe("Error in ARController::drawVideoInit().\n");
            quit(-1);
          }
        }

        int width, height;

        if (!arControllers[0]->videoParameters(
              0, &width, &height, NULL)) {
          ARLOGe("Error in ARController::videoParameters().\n");
          quit(-1);
        }

        const float contentAspectRatio = float(width * 2) / float(height);
        const float contextAspectRatio = float(contextWidth) / float(contextHeight);
        int contentWidth, contentHeight;
        if (contextAspectRatio < contentAspectRatio) {
          contentWidth = contextWidth;
          contentHeight = contentWidth / contentAspectRatio;
        } else {
          contentHeight = contextHeight;
          contentWidth = contentHeight * contentAspectRatio;
        }

        viewport[0] = (contextWidth - contentWidth) / 2;
        viewport[1] = (contextHeight - contentHeight) / 2;
        viewport[2] = contentWidth;
        viewport[3] = contentHeight;

        for (int i = 0; i <= 1; i++) {
          // Set framebuffer viewport
          if (!arControllers[i]->drawVideoSettings(
                  0, contentWidth / 2, contentHeight, false, false, false,
                  ARVideoView::HorizontalAlignment::H_ALIGN_CENTRE,
                  ARVideoView::VerticalAlignment::V_ALIGN_CENTRE,
                  ARVideoView::ScalingMode::SCALE_MODE_FIT, NULL)) {
            ARLOGe("Error in ARController::drawVideoSettings().\n");
            quit(-1);
          }
        }

        drawSetup(drawAPI, false, false, false, contentWidth / 2, contentHeight);
        drawSetViewport(viewport);

        for (int i = 0; i <= 1; i++) {
          for (int j = 0; j < markerCount; j++) {
            markerModelIDs[i][j] = drawLoadModel(NULL);
          }
        }
        contextWasUpdated = false;
      }

      for (int i = 0; i <= 1; i++) {
        // Look for trackables, and draw on each found one.
        for (int j = 0; j < markerCount; j++) {
          // Find the trackable for the given trackable ID.
          ARTrackable *marker = arControllers[i]->findTrackable(markerIDs[i][j]);
          float view[16];
          if (marker->visible) {
            // arUtilPrintMtx16(marker->transformationMatrix);
            for (int k = 0; k < 16; k++) {
              view[k] = (float)marker->transformationMatrix[k];
            }
          }
          drawSetModel(markerModelIDs[i][j], marker->visible, view);
        }
      }

      SDL_GL_MakeCurrent(gSDLWindow, gSDLContext);
      glClear(GL_COLOR_BUFFER_BIT);

      for (int i = 0; i <= 1; i++) {
        drawPrepare();

        ARdouble projectionARD[16];
        arControllers[i]->projectionMatrix(0, 10.0f, 10000.0f, projectionARD);
        for (int i = 0; i < 16; i++) {
          projection[i] = (float)projectionARD[i];
        }

        drawSetCamera(projection, NULL);

        // Clear the context.
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Display the current video frame to the current OpenGL context.
        arControllers[i]->drawVideo(0);

        draw(i);
      }

      SDL_GL_SwapWindow(gSDLWindow);
    } // if (gotFrame)
  }   // while (!done)

  free(resourcesDir);

  quit(0);
  return 0;
}

static void processCommandLineOptions(int argc, char *argv[]) {
  int i, gotTwoPartOption;

  //
  // Command-line options.
  //

  i = 1; // argv[0] is name of app, so start at 1.
  while (i < argc) {
    gotTwoPartOption = FALSE;
    // Look for two-part options first.
    if ((i + 1) < argc) {
      if (strcmp(argv[i], "--vconfl") == 0) {
        i++;
        vconfl = argv[i];
        gotTwoPartOption = TRUE;
      } else if (strcmp(argv[i], "--vconfr") == 0) {
        i++;
        vconfr = argv[i];
        gotTwoPartOption = TRUE;
      } else if (strcmp(argv[i], "--cpara") == 0) {
        i++;
        cpara = argv[i];
        gotTwoPartOption = TRUE;
      }
    }
    if (!gotTwoPartOption) {
      // Look for single-part options.
      if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-help") == 0 ||
          strcmp(argv[i], "-h") == 0) {
        usage(argv[0]);
      } else if (strcmp(argv[i], "--version") == 0 ||
                 strcmp(argv[i], "-version") == 0 ||
                 strcmp(argv[i], "-v") == 0) {
        ARPRINT("%s version %s\n", argv[0], AR_HEADER_VERSION_STRING);
        exit(0);
      } else if (strncmp(argv[i], "-loglevel=", 10) == 0) {
        if (strcmp(&(argv[i][10]), "DEBUG") == 0)
          arLogLevel = AR_LOG_LEVEL_DEBUG;
        else if (strcmp(&(argv[i][10]), "INFO") == 0)
          arLogLevel = AR_LOG_LEVEL_INFO;
        else if (strcmp(&(argv[i][10]), "WARN") == 0)
          arLogLevel = AR_LOG_LEVEL_WARN;
        else if (strcmp(&(argv[i][10]), "ERROR") == 0)
          arLogLevel = AR_LOG_LEVEL_ERROR;
        else
          usage(argv[0]);
      } else {
        ARLOGe("Error: invalid command line argument '%s'.\n", argv[i]);
        usage(argv[0]);
      }
    }
    i++;
  }
}

static void usage(char *com) {
  ARPRINT("Usage: %s [options]\n", com);
  ARPRINT("Options:\n");
  ARPRINT("  --vconfl <video parameter for the left camera>\n");
  ARPRINT("  --vconfr <video parameter for the right camera>\n");
  ARPRINT("  --cpara <camera parameter file for the camera>\n");
  ARPRINT("  --version: Print artoolkitX version and exit.\n");
  ARPRINT("  -loglevel=l: Set the log level to l, where l is one of DEBUG INFO "
          "WARN ERROR.\n");
  ARPRINT("  -h -help --help: show this message\n");
  exit(0);
}

static void quit(int rc) {
  drawCleanup();
  for (int i = 0; i <= 1; i++) {
    if (arControllers[i]) {
      arControllers[i]->drawVideoFinal(0);
      arControllers[i]->shutdown();
      delete arControllers[i];
    }
  }
  if (gSDLContext) {
    SDL_GL_MakeCurrent(0, NULL);
    SDL_GL_DeleteContext(gSDLContext);
  }
  if (gSDLWindow) {
    SDL_DestroyWindow(gSDLWindow);
  }
  SDL_Quit();
  exit(rc);
}

static void reshape(int w, int h) {
  contextWidth = w;
  contextHeight = h;
  ARLOGd("Resized to %dx%d.\n", w, h);
  contextWasUpdated = true;
}
