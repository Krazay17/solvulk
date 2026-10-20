/*
 * File: s_view2.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-11
 *
 */
#include "world.h"
#include "sol_core.h"
#include "sol_math.h"
#include "font.h"
#include "render/render.h"

static void Draw_RulesWindow(World *world, int id, float fdt, View2 *view)
{
    vec4s pos4 = {UISCALE(world->xform.draw_pos[id].x), UISCALE(world->xform.draw_pos[id].y), 0, 1.0f};
    float width;
    float height;
    forc(world, id, ScBody2)
    {
        width  = c->dims.x;
        height = c->dims.y;
    }
    RectSSBO *bg = Sol_Render_GetNext_Rect(view->layer);
    bg->flags    = 0;
    bg->pos      = pos4;
    bg->rect     = (vec4s){0, 0, UISCALE(width), UISCALE(height)};
    bg->color    = (vec4s){0, 0, 0, 0.4f};

    RectSSBO *frame  = Sol_Render_GetNext_Rect(view->layer);
    frame->flags     = 0;
    frame->pos       = pos4;
    frame->rect      = (vec4s){0, 0, UISCALE(width), UISCALE(height)};
    frame->color        = (vec4s){1, 1, 1, 0.4f};
    frame->textureId = SOL_TEXTURE_GREENFRAME;
}
static void Draw_Inventory(World *world, int id, float fdt, View2 *view)
{
    vec4s pos4 = {UISCALE(world->xform.draw_pos[id].x), UISCALE(world->xform.draw_pos[id].y), 0, 1.0f};
    float width;
    float height;
    forc(world, id, ScBody2)
    {
        width  = c->dims.x;
        height = c->dims.y;
    }
    RectSSBO *bg = Sol_Render_GetNext_Rect(view->layer);
    bg->flags    = 0;
    bg->pos      = pos4;
    bg->rect     = (vec4s){0, 0, UISCALE(width), UISCALE(height)};
    bg->color    = VEC4_BLACK;

    RectSSBO *frame  = Sol_Render_GetNext_Rect(view->layer);
    frame->flags     = 0;
    frame->pos       = pos4;
    frame->rect      = (vec4s){0, 0, UISCALE(width), UISCALE(height)};
    frame->color     = VEC4_WHITE;
    frame->textureId = SOL_TEXTURE_GREENFRAME;
}
const char *slot_text[ABILITY_SLOTS] = {"1", "2", "3", "4", "Shift", "Left", "Right"};
static void DrawAbilitybar(World *world, int id, float fdt, View2 *view)
{
    ScRef *ref = Sol_Comp_Get(world, id, ScRef);
    if (!ref)
        return;
    World *ref_world = Sol_GetWorldByIdx(ref->world);
    if (!ref_world)
        return;
    ScAbility *ability = Sol_Comp_Get(ref_world, ref->id, ScAbility);
    if (!ability)
        return;
    ScAbilitybar *abilitybar = Sol_Comp_Get(world, id, ScAbilitybar);
    if (!abilitybar)
        return;

    vec3s pos  = world->xform.draw_pos[id];
    vec4s pos4 = {pos.x, pos.y, pos.z, 1.0f};

    for (int i = 0; i < abilitybar->slot_count; i++)
    {
        int ability_slot             = abilitybar->ability_slots[i];
        const AbilityStateData *data = &ability->stateData[ability_slot];

        vec4s slot_pos   = (vec4s){UISCALE(pos.x + abilitybar->slot_dims.x * i), UISCALE(pos.y), 0, 1.0f};
        vec4s slot_rect  = (vec4s){0, 0, UISCALE(abilitybar->slot_dims.x), UISCALE(abilitybar->slot_dims.y)};
        vec4s slot_color = VEC4_WHITE;

        RectSSBO *cooldown  = Sol_Render_GetNext_Rect(view->layer);
        cooldown->extra.y   = data->cooldownRemaining > 0.0f ? data->cooldownRemaining / data->conf.cooldown : 0.0f;
        cooldown->flags     = 0b111;
        cooldown->pos       = slot_pos;
        cooldown->rect      = slot_rect;
        cooldown->textureId = SOL_TEXTURE_SWIRLFRAME;
        cooldown->color     = (vec4s){1.0f, 0.0f, 0.0f, 1.0f};

        if (ability->state[ability_slot] != 0)
        {
            RectSSBO *p  = Sol_Render_GetNext_Rect(view->layer);
            p->pos       = slot_pos;
            p->rect      = slot_rect;
            p->color     = VEC4_WHITE;
            p->textureId = SOL_TEXTURE_SWIRLFRAME;
        }

        if (data->held)
        {
            RectSSBO *p  = Sol_Render_GetNext_Rect(view->layer);
            p->pos       = slot_pos;
            p->rect      = slot_rect;
            p->color     = VEC4_WHITE;
            p->textureId = SOL_TEXTURE_SHOCKPARTICLE;
        }

        RectSSBO *frame  = Sol_Render_GetNext_Rect(view->layer + 1);
        frame->pos       = slot_pos;
        frame->rect      = (vec4s){0, 0, UISCALE(abilitybar->slot_dims.x), UISCALE(abilitybar->slot_dims.y)};
        frame->textureId = SOL_TEXTURE_GREENFRAME;
        frame->color     = VEC4_WHITE;

        Sol_Render_DrawText2D(slot_text[ability_slot], (SolFontDesc){.color  = VEC4_GREEN,
                                                                     .layer  = view->layer + 1,
                                                                     .size   = UISCALE(16.0f),
                                                                     .x      = slot_pos.x + slot_rect.z * 0.5f,
                                                                     .y      = slot_pos.y + slot_rect.w * 0.5f,
                                                                     .center = true});
    }
}
static void DrawRect(World *world, int id, float fdt, View2 *view)
{
    vec4s drawCol = view->color;
    if (Sol_Comp_Has(world, id, ScInteract))
    {
        ScInteract *interact = Sol_Comp_Get(world, id, ScInteract);

        if (interact->state & (1u << INTERACT_DOWN))
            view->downAnim = fminf(view->downAnim + fdt * 20.0f, 1.0f);
        else
            view->downAnim = fmaxf(view->downAnim - fdt * 20.0f, 0.0f);

        if (interact->state & (1u << INTERACT_HOVERED))
        {
            view->hoverAnim = fminf(view->hoverAnim + fdt * 12.0f, 1.0f);
        }
        else
            view->hoverAnim = fmaxf(view->hoverAnim - fdt * 8.0f, 0.0f);

        view->activeAnim = fmaxf(view->activeAnim - fdt * 5.0f, 0.0f);
        if ((interact->state & (1u << INTERACT_ACTIVE)))
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
    ssbo->uv         = view->textureUV;
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

static void PowerBar(World *world, int id, float fdt, View2 *view)
{
    ScRef *ref = Sol_Comp_Get(world, id, ScRef);
    if (!ref)
        return;
    World *ref_world = Sol_GetWorldByIdx(ref->world);
    if (!ref_world)
        return;
    ScCombat *combat = Sol_Comp_Get(ref_world, ref->id, ScCombat);
    if (!combat || combat->health <= 0.0f)
        return;

    vec3s pos    = world->xform.draw_pos[id];
    float speed  = -view->fillSpeed;
    float factor = 1.0f - expf(speed * fdt);
    view->targetFill =
        ref->kind == REFKIND_HEALTHBAR ? combat->health / combat->healthMax : combat->energy / combat->energyMax;
    view->fill      = Sol_Math_Lerp(view->fill, view->targetFill, factor);
    RectSSBO *ssbo  = Sol_Render_GetNext_Rect(view->layer);
    ssbo->pos       = (vec4s){UISCALE(pos.x), UISCALE(pos.y)};
    ssbo->flags     = view->flags;
    ssbo->extra.y   = view->fill;
    ssbo->extra.x   = view->border;
    ssbo->rect      = (vec4s){0, 0, UISCALE(view->dims.x), UISCALE(view->dims.y)};
    ssbo->color     = view->color;
    ssbo->textureId = view->textureID;
    ssbo->uv        = view->textureUV;
}

void (*const draw_funcs[VIEW2KIND_COUNT])(World *, int, float, View2 *) = {
    [VIEW2KIND_RECT]        = DrawRect,
    [VIEW2KIND_SLIDER]      = DrawSlider,
    [VIEW2KIND_SLIDER_FILL] = DrawSliderFill,
    [VIEW2KIND_CIRCLE]      = DrawCircle,
    [VIEW2KIND_TEXT]        = DrawText,
    [VIEW2KIND_POWERBAR]    = PowerBar,
    [VIEW2KIND_ABILITYBAR]  = DrawAbilitybar,
    [VIEW2KIND_INVENTORY]   = Draw_Inventory,
    [VIEW2KIND_RULESWINDOW] = Draw_RulesWindow,
};

void View2_Draw(World *world, double dt)
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
            if (draw_funcs[view->kind])
                draw_funcs[view->kind](world, id, fdt, view);
        }
    }
}
