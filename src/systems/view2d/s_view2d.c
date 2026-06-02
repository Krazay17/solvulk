#include "sol_core.h"

typedef void (*DrawFunc)(World *, int, double, double, CompView2d *, vec3s);

static void Step(World *world, double dt, double time);
static void Draw(World *world, double dt, double time);
static void DrawRect(World *world, int id, double dt, double time, CompView2d *view, vec3s pos);
static void DrawCircle(World *world, int id, double dt, double time, CompView2d *view, vec3s pos);
static void DrawTexture(World *world, int id, double dt, double time, CompView2d *view, vec3s pos);
static void View_DrawText(World *world, int id, double dt, double time, CompView2d *view, vec3s pos);

DrawFunc draw_funcs[VIEW2DKIND_COUNT] = {
    [VIEW2DKIND_RECT]    = DrawRect,
    [VIEW2DKIND_CIRCLE]  = DrawCircle,
    [VIEW2DKIND_TEXTURE] = DrawTexture,
    [VIEW2DKIND_TEXT]    = View_DrawText,
};

void Sol_View2d_Init(World *world)
{
    world->view2d   = calloc(MAX_ENTS, sizeof(CompView2d));
    WAdd2d(world)   = Draw;
    WAddStep(world) = Step;
}

void Sol_View2d_Add(World *world, int id, CompView2d desc)
{
    desc.fill         = 1.0f;
    desc.targetFill   = 1.0f;
    world->view2d[id] = desc;
    world->masks[id] |= HAS_VIEW2D;
}

static void Step(World *world, double dt, double time)
{
    int required = HAS_VIEW2D | HAS_OTHERWORLD;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompOtherworld *ow = &world->otherworlds[id];
        if (!ow->world || !ow->entId)
            continue;
        if (!(ow->world->masks[ow->entId] & HAS_VITAL))
            continue;

        CompVital  *v    = &ow->world->vitals[ow->entId];
        CompView2d *view = &world->view2d[id];

        float target     = v->maxHealth > 0 ? (float)v->health / (float)v->maxHealth : 0.0f;
        view->targetFill = target;
    }
}

static void Draw(World *world, double dt, double time)
{
    int required = HAS_VIEW2D;

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompView2d *view = &world->view2d[id];
        vec3s       pos;
        pos = Sol_Xform_GetDrawXform(world, id).pos;
        draw_funcs[view->kind](world, id, dt, time, view, pos);
    }
}

static void DrawRect(World *world, int id, double dt, double time, CompView2d *view, vec3s pos)
{
    float fdt = (float)dt;
    if (Sol_Interact_GetState(world, id) & INTERACT_HOVERED)
        view->hoverAnim = fminf(view->hoverAnim + dt * 12.0f, 1.0f);
    else
        view->hoverAnim = fmaxf(view->hoverAnim - dt * 8.0f, 0.0f);
    if (Sol_Interact_GetState(world, id) & INTERACT_CLICKED)
        view->clickAnim = 1.0f;
    view->clickAnim = fmaxf(view->clickAnim - dt * 5.0f, 0.0f);

    // --- DRAWING LOGIC ---
    vec4s rect    = {UISCALE(pos.x), UISCALE(pos.y), UISCALE(view->dims.x), UISCALE(view->dims.y)};
    vec4s drawCol = view->color;
    if (Sol_Interact_GetState(world, id) & INTERACT_TOGGLED)
        drawCol = view->toggleColor;
    drawCol = Sol_Color_Lerp(drawCol, view->hoverColor, view->hoverAnim);
    drawCol = Sol_Color_Lerp(drawCol, view->clickColor, view->clickAnim);

    float factor = 1.0f - expf(-8.0f * (float)dt);
    view->fill   = Sol_Math_Lerp(view->fill, view->targetFill, factor);

    float drawBorder = view->border * (1.0f + view->clickAnim);
    Sol_Render_DrawRectangle(rect, drawCol, UISCALE(drawBorder), view->fill);
}

// TODO
static void DrawCircle(World *world, int id, double dt, double time, CompView2d *view, vec3s pos)
{
}

// TODO
static void DrawTexture(World *world, int id, double dt, double time, CompView2d *view, vec3s pos)
{
}

static void View_DrawText(World *world, int id, double dt, double time, CompView2d *view, vec3s pos)
{
    if (view->text[0] != '\0')
    {
        float       textWidth = Sol_MeasureText(view->text, view->scale, SOL_FONT_ICE);
        float       cx        = pos.x - textWidth * 0.5f;
        float       cy        = pos.y + view->scale * 0.35f;
        SolFontDesc fontDesc  = {
            .str   = view->text,
            .x     = UISCALE(cx),
            .y     = UISCALE(cy),
            .size  = UISCALE(view->scale),
            .color = view->color,
            .kind  = SOL_FONT_ICE,
        };
        Sol_Render_DrawText(fontDesc);
    }
}
