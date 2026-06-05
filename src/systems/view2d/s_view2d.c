#include "sol_core.h"

typedef void (*DrawFunc)(World *, int, double, double, CompView2d *, vec3s);

static void Step(World *world, double dt, double time);
static void Draw(World *world, double dt, double time);
static void DrawRect(World *world, int id, double dt, double time, CompView2d *view, vec3s pos);
static void DrawCircle(World *world, int id, double dt, double time, CompView2d *view, vec3s pos);
static void DrawTexture(World *world, int id, double dt, double time, CompView2d *view, vec3s pos);
static void View_DrawText(World *world, int id, double dt, double time, CompView2d *view, vec3s pos);
static void DrawRectI(World *world, int id, double dt, double time, CompView2d *view, vec3s pos);

DrawFunc draw_funcs[VIEW2DKIND_COUNT] = {
    [VIEW2DKIND_RECT] = DrawRectI,      [VIEW2DKIND_CIRCLE] = DrawCircle, [VIEW2DKIND_TEXTURE] = DrawTexture,
    [VIEW2DKIND_TEXT] = View_DrawText, [VIEW2DKIND_RECTI] = DrawRectI,
};

void Sol_View2d_Init(World *world)
{
    world->view2d   = calloc(MAX_ENTS, sizeof(CompView2d));
    WAdd2d(world)   = Draw;
    WAddStep(world) = Step;
}

CompView2d *Sol_View2d_Add(World *world, int id, View2dKind kind, vec4s color, float width, float height)
{
    CompView2d view = {
        .kind       = kind,
        .color      = color,
        .dims       = {width, height},
        .fill       = 1.0f,
        .targetFill = 1.0f,
        .hoverColor = {0.5f, 0.5f, 0.5f, 1.0f},
        .clickColor = {1, 1, 1, 1},
    };
    world->view2d[id] = view;
    world->masks[id] |= HAS_VIEW2D;

    return &world->view2d[id];
}

void Sol_View2d_Set(World *world, int id, CompView2d view)
{
    world->view2d[id]            = view;
    world->view2d[id].fill       = 1.0f;
    world->view2d[id].targetFill = 1.0f;
    world->masks[id] |= HAS_VIEW2D;
}

CompView2d *Sol_View2d_Get(World *world, int id)
{
    return &world->view2d[id];
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
    for (int layer = 0; layer < UILAYER_COUNT; layer++)
        for (int i = 0; i < world->activeCount; i++)
        {
            int id = world->activeEntities[i];
            if ((world->masks[id] & required) != required)
                continue;
            CompView2d *view = &world->view2d[id];
            if (view->zindex != layer)
                continue;
            vec3s pos;
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
    drawCol = glms_vec4_lerp(drawCol, view->hoverColor, view->hoverAnim);
    drawCol = glms_vec4_lerp(drawCol, view->clickColor, view->clickAnim);

    float factor = 1.0f - expf(-8.0f * (float)dt);
    view->fill   = Sol_Math_Lerp(view->fill, view->targetFill, factor);

    float drawBorder = view->border * (1.0f + view->clickAnim);
    Sol_Render_DrawRectangle(rect, drawCol, UISCALE(drawBorder), view->fill);
}

static void DrawRectI(World *world, int id, double dt, double time, CompView2d *view, vec3s pos)
{
    float fdt = (float)dt;
    InteractState istate = Sol_Interact_GetState(world, id);

    if (istate & INTERACT_HOVERED)
        view->hoverAnim = fminf(view->hoverAnim + dt * 12.0f, 1.0f);
    else
        view->hoverAnim = fmaxf(view->hoverAnim - dt * 8.0f, 0.0f);
    if (istate & INTERACT_CLICKED)
        view->clickAnim = 1.0f;
    view->clickAnim = fmaxf(view->clickAnim - dt * 5.0f, 0.0f);

    vec4s drawCol = view->color;
    if (istate & INTERACT_TOGGLED) drawCol = view->toggleColor;
    drawCol = glms_vec4_lerp(drawCol, view->hoverColor, view->hoverAnim);
    drawCol = glms_vec4_lerp(drawCol, view->clickColor, view->clickAnim);

    float factor = 1.0f - expf(-8.0f * fdt);
    view->fill   = Sol_Math_Lerp(view->fill, view->targetFill, factor);

    RectSSBO *ssbo = Sol_Render_GetNext_Rect();
    *ssbo = (RectSSBO){0};

    float fill = view->fill;   // 1.0 = full, < 1 = progress

    ssbo->pos   = (vec4s){UISCALE(pos.x), UISCALE(pos.y),
                          (float)view->zindex / (float)UILAYER_COUNT, 1.0f};
    ssbo->dims  = (vec4s){UISCALE(view->dims.x * fill), UISCALE(view->dims.y),
                          0.0f, fill};
    ssbo->color     = drawCol;
    ssbo->textureID = view->textureID;
    ssbo->type      = (u32)(view->border * (1.0f + view->clickAnim));   // border thickness in pixels
    ssbo->uv        = (view->textureUV.x > 0.0f || view->textureUV.y > 0.0f)
                          ? (vec4s){0.0f, 0.0f, view->textureUV.x, view->textureUV.y}
                          : (vec4s){0.0f, 0.0f, 1.0f, 1.0f};

    if (view->border > 0.0f)
        ssbo->flags |= (1u << 0);   // UI_FLAG_HOLLOW
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
    if (view->text[0] == '\0') return;
    
    float textWidth = Sol_MeasureText(view->text, view->dims.x, SOL_FONT_ICE);
    printf("%s, dims: %f\n", view->text, view->dims.x);
    Sol_Render_DrawText2D((SolFontDesc){
        .str   = view->text,
        .x     = UISCALE(pos.x - textWidth * 0.5f),
        .y     = UISCALE(pos.y + view->dims.x * 0.35f),
        .size  = UISCALE(view->dims.x),
        .color = view->color,
        .kind  = SOL_FONT_ICE,
    });
}

void Sol_View2d_SetText(World *world, int id, const char *text)
{
    strncpy(world->view2d[id].text, text, 64);
}
