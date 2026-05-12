/*
 * File: sol.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * ==== Sol Blade ====
 */

#pragma once

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sol/types.h"
#include "world.h"

#define SOL_VERSION 1.0

#define MAX_SYSTEMS 64
#define MAX_ENTS (1 << 15)

SOLAPI void      Sol_Init(void *hwnd, void *hInstance);
SOLAPI void      Sol_Tick(double dt, double time);
SOLAPI void      Sol_Destroy();
SOLAPI SolState *Sol_GetState();
SOLAPI double    Sol_GetGameTime();
SOLAPI void      Sol_Window_Resize(float width, float height);

SOLAPI void     Sol_Input_OnKey(int vkCode, bool down);
SOLAPI void     Sol_Input_OnMouseMove(int x, int y);
SOLAPI void     Sol_Input_OnMouseButton(SolMouseButton btn, bool down);
SOLAPI void     Sol_Input_OnMouseWheel(int delta);
SOLAPI void     Sol_Input_OnRawMouse(int x, int y);
SOLAPI void     Sol_Input_Update();
SOLAPI bool     Sol_Input_KeyDown(SolKey key);
SOLAPI bool     Sol_Input_KeyPressed(SolKey key); // true only on frame of press
SOLAPI SolMouse Sol_Input_GetMouse();
SOLAPI SolLook *Sol_Input_GetLook();

SOLAPI void Sol_Debug_Add(const char *text, float value);

SOLAPI float Sol_MeasureText(const char *str, float size, SolFontKind id);

SolRayResult Sol_Raycast(World *world, SolRay ray);
SolRayResult Sol_RaycastD(World *world, SolRay ray, float debugDuration);

typedef enum
{
    SOL_AUDIO_BEEP1,
    SOL_AUDIO_BEEP2,
    SOL_AUDIO_DIGILOAD,
    SOL_AUDIO_HIT,
    SOL_AUDIO_MENUMUSIC,
    SOL_AUDIO_SPACEGUN,
    SOL_AUDIO_WOONG,
    SOL_AUDIO_COUNT,
} SolAudioId;
void Sol_Audio_Play(SolAudioId id);
void Sol_Audio_PlayAt(SolAudioId id, vec3s pos);

float Sol_Render_GetAspect(void);
void  Sol_Render_DrawLine(SolLine *lines, int count);
void  Sol_Render_DrawRectangle(vec4s rect, vec4s color, float thickness);
void  Sol_Render_DrawText(SolFontDesc desc);

typedef struct
{
    vec4s pos;
    vec4s color;
    vec4s params;
} SphereDesc;
void Sol_Render_PushSphere(SphereDesc desc);

typedef enum
{
    BILLBOARD_HEALTHBAR,
    BILLBOARD_ICON,
    BILLBOARD_DAMAGE_NUMBER,
} BillboardKind;
typedef struct
{
    BillboardKind kind;
    vec4s         pos;
    vec4s         color;
    vec4s         params;
    u32           flags;
} BillboardDesc;
void Sol_Render_PushBillboard(BillboardDesc desc);

typedef struct
{
    vec4s   pos;
    versors rotation;
    vec4s   color;
    vec4s   uv;
} QuadDesc;
void Sol_Render_PushQuad(QuadDesc desc);

typedef struct ModelPushDesc
{
    SolModelId handle;
    vec4s      position;
    vec4s      scale;
    vec4s      rotation;
    vec4s      color;
    vec4s      material;
    u32        flags;
    bool       hasAnim;
    mat4      *bones;
} ModelPushDesc;
void Sol_Render_PushModel(ModelPushDesc desc);

static inline int Sol_Realloc(void **data, int count, int *capacity, size_t size)
{
    if (count >= *capacity)
    {
        int   newCap = (*capacity <= 0) ? 128 : *capacity * 2;
        void *tmp    = realloc(*data, size * newCap);
        if (!tmp)
        {
            fprintf(stderr, "Failed to realloc");
            return 1;
        }
        *data     = tmp;
        *capacity = newCap;
    }
    return 0;
}