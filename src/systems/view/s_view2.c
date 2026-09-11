#include "world.h"
#include "sol_core.h"
#include "sol_math.h"
#include "render/render.h"

typedef void (*DrawFunc)(World *, int, float, View2 *, vec3s, u32);

static void DrawRect(World *world, int id, float fdt, View2 *view, vec3s pos, u32 layer);
static void DrawSliderFill(World *world, int id, float fdt, View2 *view, vec3s pos, u32 layer);
static void DrawSlider(World *world, int id, float fdt, View2 *view, vec3s pos, u32 layer);
static void DrawCircle(World *world, int id, float fdt, View2 *view, vec3s pos, u32 layer);
static void DrawText(World *world, int id, float fdt, View2 *view, vec3s pos, u32 layer);

DrawFunc draw_funcs[VIEW2KIND_COUNT] = {
    [VIEW2KIND_RECT]   = DrawRect,
    [VIEW2KIND_SLIDER] = DrawSlider,
    [VIEW2KIND_SLIDER_FILL] = DrawSliderFill,
    [VIEW2KIND_CIRCLE] = DrawCircle,
    [VIEW2KIND_TEXT]   = DrawText,
};

void Sol_View2d_Init(World *world)
{
}

void View2_Draw(World *world, double dt)
{
    float fdt              = (float)dt;
    SparseSet_ScView2 *set = Sol_Comp_Set(world, ScView2);
    for (int i = 0; i < set->cnt; i++)
    {
        int id            = set->dense[i];
        ScView2 *viewComp = &set->data[i];

        for (int j = 0; j < viewComp->count; j++)
        {
            View2 *view = &viewComp->views[j];
            draw_funcs[view->kind](world, id, fdt, view, world->xform.draw_pos[id], viewComp->layer);
        }
    }
}

static void DrawRect(World *world, int id, float fdt, View2 *view, vec3s pos, u32 layer)
{
    vec4s drawCol = view->color;
    if (Sol_Comp_Has(world, id, ScInteract))
    {
        ScInteract *interact = Sol_Comp_Get(world, id, ScInteract);

        if (interact->state & INTERACT_DOWN)
            view->downAnim = fminf(view->downAnim + fdt * 20.0f, 1.0f);
        else
            view->downAnim = fmaxf(view->downAnim - fdt * 20.0f, 0.0f);

        if (interact->state & INTERACT_HOVERED)
            view->hoverAnim = fminf(view->hoverAnim + fdt * 12.0f, 1.0f);
        else
            view->hoverAnim = fmaxf(view->hoverAnim - fdt * 8.0f, 0.0f);

        if ((interact->state & INTERACT_JUSTUP) && !(interact->state_prev & INTERACT_DRAGGING))
            view->clickAnim = 1.0f;

        view->clickAnim = fmaxf(view->clickAnim - fdt * 5.0f, 0.0f);

        if (interact->state & INTERACT_TOGGLED)
            drawCol = view->toggleColor;
    }
    drawCol          = glms_vec4_lerp(drawCol, view->hoverColor, view->hoverAnim);
    drawCol          = glms_vec4_lerp(drawCol, view->downColor, view->downAnim);
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

static void DrawSliderFill(World *world, int id, float fdt, View2 *view, vec3s pos, u32 layer)
{
    ScSlider *slider = Sol_Comp_Get(world, id, ScSlider);
    if (!slider)
        return;

    float t = slider->value;

    float track_origin = pos.x + view->offset.x;

    float right = view->dims.x * t;
    float y = pos.y + view->offset.y;

    RectSSBO *ssbo  = Sol_Render_GetNext_Rect(layer);
    ssbo->rect      = (vec4s){UISCALE(track_origin), UISCALE(y), UISCALE(right), UISCALE(view->dims.y)};
    ssbo->scale     = 1.0f;
    ssbo->fill      = 1.0f;
    ssbo->color     = view->color;
    ssbo->flags     = view->flags;
    ssbo->textureID = view->textureID;
    ssbo->border    = view->border * (1.0f + view->clickAnim);
    ssbo->uv        = (view->textureUV.x > 0.0f || view->textureUV.y > 0.0f)
                          ? (vec4s){0.0f, 0.0f, view->textureUV.x, view->textureUV.y}
                          : (vec4s){0.0f, 0.0f, 1.0f, 1.0f};
}

static void DrawSlider(World *world, int id, float fdt, View2 *view, vec3s pos, u32 layer)
{
    ScSlider *slider = Sol_Comp_Get(world, id, ScSlider);
    if (!slider)
        return;

    float t = slider->value;

    float width        = view->dims.x * 0.15f;
    float track_origin = pos.x + view->offset.x;
    float travel_range = view->dims.x - width;

    // Position of handle's left edge
    float x = track_origin + (travel_range * t);
    float y = pos.y + view->offset.y;

    RectSSBO *ssbo  = Sol_Render_GetNext_Rect(layer);
    ssbo->rect      = (vec4s){UISCALE(x), UISCALE(y), UISCALE(width), UISCALE(view->dims.y)};
    ssbo->scale     = 1.0f;
    ssbo->fill      = 1.0f;
    ssbo->color     = view->color;
    ssbo->flags     = view->flags;
    ssbo->textureID = view->textureID;
    ssbo->border    = view->border * (1.0f + view->clickAnim);
    ssbo->uv        = (view->textureUV.x > 0.0f || view->textureUV.y > 0.0f)
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
    Sol_Render_DrawText2D(view->text, (SolFontDesc){
                                          .layer = layer,
                                          .x     = UISCALE(pos.x + view->offset.x - textWidth * 0.5f),
                                          .y     = UISCALE(pos.y + view->offset.y + view->dims.x * 0.35f),
                                          .size  = UISCALE(view->dims.x),
                                          .color = view->color,
                                          .kind  = SOL_FONT_ICE,
                                      });
}
