//
//  draw.h
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

#ifndef __draw_h__
#define __draw_h__

#include <ARX/AR/config.h>
#include <ARX/ARG/arg.h>
#include <stdint.h>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

// Inititialize necessary data, to be called once at startup
void drawInit();
// Toggle the drawing of virtual models
void drawToggleModels();
// Recreate OpenGL context and buffers
void drawSetup(ARG_API drawAPI_in, bool rotate90_in, bool flipH_in, bool flipV_in, int width, int height);
// Update objects requiring frame data (disparity/view space maps)
void drawUpdate(int width, int height, int contentWidth, int contentHeight, std::vector<unsigned char> frames[2]);
// Enable framebuffer and viewport for capturing frames and virtual models on a texture
void drawPrepare();
// Load virtual models
int drawLoadModel();
// Set viewport
void drawSetViewport(int32_t viewport[4]);
// Set projection and view matrix
void drawSetCamera(float projection[16], float view[16]);
// Set visibility and pose of virtual model
void drawSetModel(int modelIndex, bool visible, float pose[16]);
// Draw virtual model and stylize resulting texture, to be called after drawUpdate
void draw(size_t index, int width, int height, int contentWidth, int contentHeight);
// Cleanup OpenGL
void drawCleanup();

#ifdef __cplusplus
}
#endif
#endif // !__draw_h__

