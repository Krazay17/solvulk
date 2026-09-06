#include "world.h"
#include "sol_core.h"
#include "sol_math.h"
#include "render/render.h"

typedef void (*DrawFunc)(World *, int, float, View2 *, vec3s, u32);

static void DrawRect(World *world, int id, float fdt, View2 *view, vec3s pos, u32 layer);
static void DrawCircle(World *world, int id, float fdt, View2 *view, vec3s pos, u32 layer);
static void DrawText(World *world, int id, float fdt, View2 *view, vec3s pos, u32 layer);

DrawFunc draw_funcs[VIEW2DKIND_COUNT] = {
    [VIEW2DKIND_RECT]   = DrawRect,
    [VIEW2DKIND_CIRCLE] = DrawCircle,
    [VIEW2DKIND_TEXT]   = DrawText,
};

void Sol_View2d_Init(World *world)
{
}

void View2_Draw(World *world, double dt)
{
    float              fdt = (float)dt;
    SparseSet_ScView2 *set = Sol_Comp_Set(world, ScView2);
    for (int i = 0; i < set->cnt; i++)
    {
        int      id       = set->dense[i];
        ScView2 *viewComp = &set->data[i];

        for (int j = 0; j < viewComp->count; j++)
        {
            View2   *view  = &viewComp->views[j];
            ScXform *xform = Sol_Comp_Get(world, id, ScXform);
            if (!xform)
                continue;
            draw_funcs[view->kind](world, id, fdt, view, xform->draw_pos, viewComp->layer);
        }
    }
}

void View2_Healthbar(World *world, double dt)
{
    SparseSet_ScView2 *set = Sol_Comp_Set(world, ScView2);
    for (int i = 0; i < set->cnt; i++)
    {
        int      id   = set->dense[i];
        ScView2 *view = &set->data[i];

        if (!Sol_Comp_Has(world, id, ScTracker))
            continue;
        ScTracker *tracker = Sol_Comp_Get(world, id, ScTracker);
        if (!tracker->world || !tracker->entId)
            continue;
        if (!Sol_Comp_Has(world, id, ScCombat))
            continue;
        ScCombat *combat          = Sol_Comp_Get(tracker->world, tracker->entId, ScCombat);
        float     target          = combat->maxHealth > 0 ? combat->health / combat->maxHealth : 0.0f;
        view->views[2].targetFill = target;
        view->views[3].fill = view->views[3].targetFill = target;
    }
}

void View2_Abilitybar(World *world, double dt)
{
    SparseSet_ScView2 *set = Sol_Comp_Set(world, ScView2);
    for (int i = 0; i < set->cnt; i++)
    {
        int      id   = set->dense[i];
        ScView2 *view = &set->data[i];

        if (!Sol_Comp_Has(world, id, ScTracker))
            continue;

        ScTracker *tracker = Sol_Comp_Get(world, id, ScTracker);
        if (!tracker->getters[0] || !tracker->getters[1])
            continue;

        float target              = tracker->getters[0](tracker->world, tracker->entId) > 0
                                        ? tracker->getters[0](tracker->world, tracker->entId) /
                                              tracker->getters[1](tracker->world, tracker->entId)
                                        : 0.0f;
        view->views[2].targetFill = target;
        view->views[3].fill = view->views[3].targetFill = target;
    }
}

static void DrawRect(World *world, int id, float fdt, View2 *view, vec3s pos, u32 layer)
{
    vec4s drawCol = view->color;
    if (Sol_Comp_Has(world, id, ScInteract))
    {
        ScInteract *interact = Sol_Comp_Get(world, id, ScInteract);
        if (interact->state & INTERACT_HOVERED)
            view->hoverAnim = fminf(view->hoverAnim + fdt * 12.0f, 1.0f);
        else
            view->hoverAnim = fmaxf(view->hoverAnim - fdt * 8.0f, 0.0f);
        if (interact->state & INTERACT_PRESSED)
            view->clickAnim = 1.0f;
        view->clickAnim = fmaxf(view->clickAnim - fdt * 5.0f, 0.0f);

        if (interact->state & INTERACT_TOGGLED)
            drawCol = view->toggleColor;
    }
    drawCol          = glms_vec4_lerp(drawCol, view->hoverColor, view->hoverAnim);
    drawCol          = glms_vec4_lerp(drawCol, view->clickColor, view->clickAnim);
    float speed      = view->fillSpeed > 0 ? -view->fillSpeed : -14.0f;
    float factor     = 1.0f - expf(speed * fdt);
    view->targetFill = view->targetFill == 0 ? 1.0f : view->targetFill;
    view->fill       = Sol_Math_Lerp(view->fill, view->targetFill, factor);
    RectSSBO *ssbo   = Sol_Render_GetNext_Rect(layer);
    *ssbo            = (RectSSBO){0};
    ssbo->rect       = (vec4s){UISCALE(pos.x + view->offset.x), UISCALE(pos.y + view->offset.y), UISCALE(view->dims.x),
                               UISCALE(view->dims.y)};
    ssbo->scale      = 1.0f;
    ssbo->fill       = view->fill;
    ssbo->color      = drawCol;
    ssbo->flags      = view->flags;
    ssbo->textureID  = view->textureID;
    ssbo->border     = view->border * (1.0f + view->clickAnim); // border thickness in pixels
    ssbo->uv         = (view->textureUV.x > 0.0f || view->textureUV.y > 0.0f)
                           ? (vec4s){0.0f, 0.0f, view->textureUV.x, view->textureUV.y}
                           : (vec4s){0.0f, 0.0f, 1.0f, 1.0f};
}

static void DrawCircle(World *world, int id, float fdt, View2 *view, vec3s pos, u32 layer)
{
}

static void DrawText(World *world, int id, float fdt, View2 *view, vec3s pos, u32 layer)
{
    if (view->text[0] == '\0')
        return;

    float textWidth = Sol_MeasureText(view->text, view->dims.x, SOL_FONT_ICE);
    Sol_Render_DrawText2D((SolFontDesc){
        .layer = layer,
        .str   = view->text,
        .x     = UISCALE(pos.x + view->offset.x - textWidth * 0.5f),
        .y     = UISCALE(pos.y + view->offset.y + view->dims.x * 0.35f),
        .size  = UISCALE(view->dims.x),
        .color = view->color,
        .kind  = SOL_FONT_ICE,
    });
}

void Sol_View2d_SetText(World *world, int id, View2 *view, const char *text)
{
    strncpy(view->text, text, 64);
}
