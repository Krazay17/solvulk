#include "world.h"
#include "sol_core.h"
#include "sol_math.h"
#include "render/render.h"

static void DrawAbilitybar(World *world, int id, float fdt, View2 *view)
{
    if (!Sol_Comp_Has(world, id, ScRef) || !Sol_Comp_Has(world, id, ScAbilitybar))
        return;
    ScAbilitybar *abilitybar = Sol_Comp_Get(world, id, ScAbilitybar);
    ScRef *ref               = Sol_Comp_Get(world, id, ScRef);
    World *ref_world         = Sol_GetWorldByIdx(ref->ent_world);
    if (!ref_world)
        return;
    ScAbility *ability = Sol_Comp_Get(Sol_GetWorldByIdx(ref->ent_world), ref->ent_id, ScAbility);
    if (!ability)
        return;
    vec3s pos = world->xform.draw_pos[id];

    int count   = abilitybar->slots;
    float width = view->dims.x / (float)count;
    float fill  = 0;
    for (int i = 0; i < count; i++)
    {
        RectSSBO *rect  = Sol_Render_GetNext_Rect(view->layer);
        u32 texture = view->textureID;
        if (view->layer == 0)
        {
            texture = ability_texture_map[ability->base_actions[i]];
        }
        rect->extra.z = view->desat;
        fill            = ability->stateData[i].cooldownRemaining > 0.0f
                              ? ability->stateData[i].cooldownRemaining / ability->stateData[i].conf.cooldown
                              : 0.0f;
        rect->flags     = view->flags;
        rect->extra.y   = fill;
        rect->pos       = (vec4s){UISCALE(pos.x + width * i), UISCALE(pos.y)};
        rect->rect      = (vec4s){0, 0, UISCALE(width), UISCALE(view->dims.y)};
        rect->textureId = texture;
        rect->color     = view->color;
        rect->uv        = view->textureUV;
    }
}

static void DrawRect(World *world, int id, float fdt, View2 *view)
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

        view->activeAnim = fmaxf(view->activeAnim - fdt * 5.0f, 0.0f);
        if ((interact->state & (INTERACT_TOGGLED | INTERACT_ACTIVE)))
            view->activeAnim = 1.0f;
    }
    drawCol          = glms_vec4_lerp(drawCol, view->hoverColor, view->hoverAnim);
    drawCol          = glms_vec4_lerp(drawCol, view->downColor, view->downAnim);
    drawCol          = glms_vec4_lerp(drawCol, view->activeColor, view->activeAnim);
    float speed      = view->fillSpeed > 0 ? -view->fillSpeed : -14.0f;
    float factor     = 1.0f - expf(speed * fdt);
    view->targetFill = view->targetFill == 0 ? 1.0f : view->targetFill;
    view->fill       = Sol_Math_Lerp(view->fill, view->targetFill, factor);
    vec3s pos        = world->xform.draw_pos[id];
    RectSSBO *ssbo   = Sol_Render_GetNext_Rect(view->layer);
    ssbo->pos        = (vec4s){UISCALE(pos.x + view->offset.x), UISCALE(pos.y + view->offset.y), 0, 1.0f};
    ssbo->rect       = (vec4s){0, 0, UISCALE(view->dims.x), UISCALE(view->dims.y)};
    ssbo->color      = drawCol;
    ssbo->flags      = view->flags;
    ssbo->textureId  = view->textureID;
    ssbo->extra.x    = view->border * (1.0f + view->activeAnim); // border thickness in pixels
    ssbo->extra.y    = view->fill;
    ssbo->uv         = view->textureUV.z > 0 ? view->textureUV : (vec4s){0, 0, 1, 1};
}

static void DrawSliderFill(World *world, int id, float fdt, View2 *view)
{
    ScSlider *slider = Sol_Comp_Get(world, id, ScSlider);
    if (!slider)
        return;
    vec3s pos = world->xform.draw_pos[id];

    float t            = slider->value;
    float track_origin = pos.x + view->offset.x;
    float y            = pos.y + view->offset.y;
    float right        = view->dims.x * t;

    RectSSBO *ssbo  = Sol_Render_GetNext_Rect(view->layer);
    ssbo->pos       = (vec4s){UISCALE(track_origin), UISCALE(pos.y), pos.z, 1.0f};
    ssbo->rect      = (vec4s){0, 0, UISCALE(right), UISCALE(view->dims.y)};
    ssbo->color     = view->color;
    ssbo->flags     = view->flags;
    ssbo->textureId = view->textureID;
}

static void DrawSlider(World *world, int id, float fdt, View2 *view)
{
    ScSlider *slider = Sol_Comp_Get(world, id, ScSlider);
    if (!slider)
        return;

    vec3s pos = world->xform.draw_pos[id];
    float t   = slider->value;

    float width        = view->dims.x * 0.15f;
    float track_origin = pos.x + view->offset.x;
    float travel_range = view->dims.x - width;

    // Position of handle's left edge
    float x = track_origin + (travel_range * t);
    float y = pos.y + view->offset.y;

    RectSSBO *ssbo  = Sol_Render_GetNext_Rect(view->layer);
    ssbo->pos       = (vec4s){UISCALE(x), UISCALE(y), UISCALE(pos.z), 1.0f};
    ssbo->rect      = (vec4s){0, 0, UISCALE(width), UISCALE(view->dims.y)};
    ssbo->color     = view->color;
    ssbo->flags     = view->flags;
    ssbo->textureId = view->textureID;
    ssbo->extra.x   = view->border * (1.0f + view->activeAnim);
}

static void DrawCircle(World *world, int id, float fdt, View2 *view)
{
}

static void DrawText(World *world, int id, float fdt, View2 *view)
{
    if (view->text[0] == '\0')
        return;
    vec3s pos = world->xform.draw_pos[id];

    float textWidth = Sol_MeasureText(view->text, view->dims.x, SOL_FONT_ICE);
    Sol_Render_DrawText2D(view->text, (SolFontDesc){
                                          .layer = view->layer,
                                          .x     = UISCALE(pos.x + view->offset.x - textWidth * 0.5f),
                                          .y     = UISCALE(pos.y + view->offset.y + view->dims.x * 0.35f),
                                          .size  = UISCALE(view->dims.x),
                                          .color = view->color,
                                          .kind  = SOL_FONT_ICE,
                                      });
}

static void DrawHealthbar(World *world, int id, float fdt, View2 *view)
{
    ScRef *ref       = Sol_Comp_Get(world, id, ScRef);
    ScCombat *combat = Sol_Comp_Get(Sol_GetWorldByIdx(ref->ent_world), ref->ent_id, ScCombat);
    if (!ref || !combat || combat->health <= 0.0f)
        return;
    vec3s pos        = world->xform.draw_pos[id];
    float speed      = -view->fillSpeed;
    float factor     = 1.0f - expf(speed * fdt);
    view->targetFill = combat->health / combat->healthMax;
    view->fill       = Sol_Math_Lerp(view->fill, view->targetFill, factor);
    RectSSBO *ssbo   = Sol_Render_GetNext_Rect(view->layer);
    ssbo->pos        = (vec4s){UISCALE(pos.x), UISCALE(pos.y)};
    ssbo->flags      = 1;
    ssbo->extra.y    = view->fill;
    ssbo->extra.x    = view->border;
    ssbo->rect       = (vec4s){0, 0, UISCALE(view->dims.x), UISCALE(view->dims.y)};
    ssbo->color      = view->color;
    ssbo->textureId  = view->textureID;
    ssbo->uv         = view->textureUV;
}

typedef void (*DrawFunc)(World *, int, float, View2 *);
DrawFunc draw_funcs[VIEW2KIND_COUNT] = {
    [VIEW2KIND_RECT]        = DrawRect,
    [VIEW2KIND_SLIDER]      = DrawSlider,
    [VIEW2KIND_SLIDER_FILL] = DrawSliderFill,
    [VIEW2KIND_CIRCLE]      = DrawCircle,
    [VIEW2KIND_TEXT]        = DrawText,
    [VIEW2KIND_HEALTHBAR]   = DrawHealthbar,
    [VIEW2KIND_ABILITYBAR]  = DrawAbilitybar,
};

void View2_Draw(World *world)
{
    float fdt              = world->fdt;
    SparseSet_ScView2 *set = Sol_Comp_Set(world, ScView2);
    for (int i = 0; i < set->cnt; i++)
    {
        int id            = set->dense[i];
        ScView2 *viewComp = &set->data[i];
        for (int j = 0; j < viewComp->count; j++)
        {
            View2 *view = &viewComp->views[j];
            draw_funcs[view->kind](world, id, fdt, view);
        }
    }
}
