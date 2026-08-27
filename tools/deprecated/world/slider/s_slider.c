#include "s_slider.h"
#include "world.h"

#include "xform/s_xform.h"
#include "render/render.h"
#include "physx/s_body2d.h"

typedef struct
{
    int         cnt, cap;
    int        *sparse, *dense;
    CompSlider *sliders;
} WorldSliders;

static void Slider_Tick(World *world, double dt, double time)
{
}

static void Slider_Draw(World *world, double dt, double time)
{
    WorldSliders *wc = world->dense_components[WORLD_SYS_SLIDER];
    for (int i = 0; i < wc->cnt; i++)
    {
        int         id     = wc->dense[i];
        CompSlider *slider = &wc->sliders[i];
        CompXform  *xform  = &world->xforms[id];
        CompBody2d *body   = &world->body2d[id];

        RectSSBO *rect = Sol_Render_GetNext_Rect();
        rect->scale    = 1.0f;
        rect->fill     = 1.0f;
        rect->color    = VEC4_RED;
        rect->rect.x   = (xform->drawPos.x - slider->handle_width * 0.5f) + body->dims.x * slider->value;
        rect->rect.y   = xform->drawPos.y;
        rect->rect.z   = slider->handle_width;
        rect->rect.w   = slider->handle_height;
    }
}

void Sol_Slider_Init(World *world)
{
    WorldSliders *wc                          = malloc(sizeof(WorldSliders));
    world->dense_components[WORLD_SYS_SLIDER] = wc;

    wc->cap     = 32;
    wc->cnt     = 0;
    wc->sparse  = malloc(MAX_ENTS * sizeof(int));
    wc->dense   = malloc(wc->cap * sizeof(int));
    wc->sliders = malloc(wc->cap * sizeof(CompSlider));
    memset(wc->sparse, -1, MAX_ENTS * sizeof(int));

    // WAddTick(world) = Slider_Tick;
    // WAdd2d(world)   = Slider_Draw;
}

CompSlider *Sol_Slider_Add(World *world, int id)
{
    WorldSliders *wc = world->dense_components[WORLD_SYS_SLIDER];
    if (wc->sparse[id] != -1)
        return &wc->sliders[wc->sparse[id]];
    if (wc->cnt >= wc->cap)
    {
        wc->cap *= 2;
        wc->dense   = realloc(wc->dense, wc->cap * sizeof(int));
        wc->sliders = realloc(wc->sliders, wc->cap * sizeof(CompSlider));
    }
    int denseIdx          = wc->cnt++;
    wc->sparse[id]        = denseIdx;
    wc->dense[denseIdx]   = id;
    wc->sliders[denseIdx] = (CompSlider){0};

    return &wc->sliders[denseIdx];
}

CompSlider *Sol_Slider_Get(World *world, int id)
{
    WorldSliders *wc = world->dense_components[WORLD_SYS_SLIDER];
    if (wc->sparse[id] != -1)
        return &wc->sliders[wc->sparse[id]];
    return NULL;
}

bool Sol_Slider_Has(World *world, int id)
{
    WorldSliders *wc = world->dense_components[WORLD_SYS_SLIDER];
    return wc->sparse[id] != -1;
}

void Sol_Slider_Rem(World *world, int id)
{
    WorldSliders *wc  = world->dense_components[WORLD_SYS_SLIDER];
    int           idx = wc->sparse[id];
    if (idx < 0)
        return;

    int lastIdx        = wc->cnt - 1;
    int lastId         = wc->dense[lastIdx];
    wc->sliders[idx]   = wc->sliders[lastIdx];
    wc->dense[idx]     = lastId;
    wc->sparse[lastId] = idx;
    wc->sparse[id]     = -1;
    wc->cnt--;
}