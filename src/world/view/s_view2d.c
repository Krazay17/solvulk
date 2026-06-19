#include "s_view2d.h"
#include "sol_core.h"
#include "world.h"
#include "sol_math.h"
#include "xform/s_xform.h"
#include "render/render.h"
#include "vital/s_vital.h"

typedef void (*DrawFunc)(World *, int, double, double, SolView2d *, vec3s);

static void PlayerHealthbar(World *world, double dt, double time);
static void Draw(World *world, double dt, double time);
static void DrawRect(World *world, int id, double dt, double time, SolView2d *view, vec3s pos);
static void DrawCircle(World *world, int id, double dt, double time, SolView2d *view, vec3s pos);
static void View_DrawText(World *world, int id, double dt, double time, SolView2d *view, vec3s pos);

DrawFunc draw_funcs[VIEW2DKIND_COUNT] = {
    [VIEW2DKIND_RECT] = DrawRect,
    [VIEW2DKIND_TEXT] = View_DrawText,
};

void Sol_View2d_Init(World *world)
{
    world->view2d   = calloc(MAX_ENTS, sizeof(CompView2d));
    WAdd2d(world)   = Draw;
    WAddStep(world) = PlayerHealthbar;
}

SolView2d *Sol_View2d_Add(World *world, int id, View2dKind kind, vec4s color, float width, float height)
{
    SolView2d view = {
        .kind       = kind,
        .color      = color,
        .dims       = {width, height},
        .fill       = 1.0f,
        .targetFill = 1.0f,
        .hoverColor = {0.0f, 0.0f, 0.0f, 0.0f},
        .clickColor = {0.0f, 0.0f, 0.0f, 0.0f},
    };
    CompView2d *compView = &world->view2d[id];

    if (compView->count >= MAX_VIEWS)
        return NULL;
    int idx = compView->count;
    compView->count++;
    compView->views[idx] = view;
    world->masks[id] |= HAS_VIEW2D;

    return &world->view2d[id].views[idx];
}

void Sol_View2d_Set(World *world, int id, CompView2d view)
{
    world->view2d[id] = view;
    world->masks[id] |= HAS_VIEW2D;
}

CompView2d *Sol_View2d_Get(World *world, int id)
{
    return &world->view2d[id];
}

static void PlayerHealthbar(World *world, double dt, double time)
{
    int required = HAS_VIEW2D | HAS_TRACKER;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        if (!(world->flags[id].flags & EFLAG_HEALTHBAR))
            continue;
        CompTracker *tracker = &world->trackers[id];
        if (!tracker->world || !tracker->entId)
            continue;
        if (!(tracker->world->masks[tracker->entId] & HAS_VITAL))
            continue;
        CompVital  *v    = &tracker->world->vitals[tracker->entId];
        CompView2d *view = &world->view2d[id];

        float target              = v->maxHealth > 0 ? v->health / v->maxHealth : 0.0f;
        view->views[2].targetFill = target;
        view->views[3].fill = view->views[3].targetFill = target;
    }
}

static void Draw(World *world, double dt, double time)
{
    int required = HAS_VIEW2D;
    for (int layer = 0; layer < UILAYER_COUNT; layer++)
        for (int i = 0; i < world->activeCount; i++)
        {
            int id = world->activeEntities[i];
            if ((world->masks[id] & required) != required)
                continue;
            CompView2d *viewComp = &world->view2d[id];
            for (int j = 0; j < viewComp->count; j++)
            {
                SolView2d *view = &viewComp->views[j];
                if (view->zindex != layer)
                    continue;
                vec3s pos;
                pos = Sol_Xform_GetDrawXform(world, id).pos;
                draw_funcs[view->kind](world, id, dt, time, view, pos);
            }
        }
}

static void DrawRect(World *world, int id, double dt, double time, SolView2d *view, vec3s pos)
{
    float         fdt    = (float)dt;
    InteractState istate = Sol_Interact_GetState(world, id);

    if (istate & INTERACT_HOVERED)
        view->hoverAnim = fminf(view->hoverAnim + dt * 12.0f, 1.0f);
    else
        view->hoverAnim = fmaxf(view->hoverAnim - dt * 8.0f, 0.0f);
    if (istate & INTERACT_CLICKED)
        view->clickAnim = 1.0f;
    view->clickAnim = fmaxf(view->clickAnim - dt * 5.0f, 0.0f);

    vec4s drawCol = view->color;
    if (istate & INTERACT_TOGGLED)
        drawCol = view->toggleColor;
    drawCol      = glms_vec4_lerp(drawCol, view->hoverColor, view->hoverAnim);
    drawCol      = glms_vec4_lerp(drawCol, view->clickColor, view->clickAnim);
    float speed  = view->fillSpeed > 0 ? -view->fillSpeed : -14.0f;
    float factor = 1.0f - expf(speed * fdt);
    view->fill   = Sol_Math_Lerp(view->fill, view->targetFill, factor);

    RectSSBO *ssbo = Sol_Render_GetNext_Rect();
    *ssbo          = (RectSSBO){0};
    ssbo->pos      = (vec4s){
        UISCALE(pos.x + view->offset.x),
        UISCALE(pos.y + view->offset.y),
        (float)view->zindex / (float)UILAYER_COUNT,
        1.0f,
    };
    ssbo->dims      = (vec4s){UISCALE(view->dims.x), UISCALE(view->dims.y), view->dims.z, view->fill};
    ssbo->color     = drawCol;
    ssbo->flags     = view->flags;
    ssbo->textureID = view->textureID;
    ssbo->type      = (u32)(view->border * (1.0f + view->clickAnim)); // border thickness in pixels
    ssbo->uv        = (view->textureUV.x > 0.0f || view->textureUV.y > 0.0f)
                          ? (vec4s){0.0f, 0.0f, view->textureUV.x, view->textureUV.y}
                          : (vec4s){0.0f, 0.0f, 1.0f, 1.0f};

    if (view->border > 0.0f)
        ssbo->flags |= (1u << 0); // UI_FLAG_HOLLOW
}

// TODO
static void DrawCircle(World *world, int id, double dt, double time, SolView2d *view, vec3s pos)
{
}

static void View_DrawText(World *world, int id, double dt, double time, SolView2d *view, vec3s pos)
{
    if (view->text[0] == '\0')
        return;

    float textWidth = Sol_MeasureText(view->text, view->dims.x, SOL_FONT_ICE);
    Sol_Render_DrawText2D((SolFontDesc){
        .str   = view->text,
        .x     = UISCALE(pos.x + view->offset.x - textWidth * 0.5f),
        .y     = UISCALE(pos.y + view->offset.y + view->dims.x * 0.35f),
        .size  = UISCALE(view->dims.x),
        .color = view->color,
        .kind  = SOL_FONT_ICE,
    });
}

void Sol_View2d_SetText(World *world, int id, SolView2d *view, const char *text)
{
    strncpy(view->text, text, 64);
}
