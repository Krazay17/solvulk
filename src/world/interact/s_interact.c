/*
 * File: s_interact.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Interact!
 */
#include "s_interact.h"
#include "sol_core.h"
#include "world.h"
#include "sol_math.h"
#include "input.h"
#include "render/render.h"

#include "xform/s_xform.h"
#include "physx/s_body.h"
#include "physx/s_body2d.h"
#include "view/s_view2d.h"
#include "parent/s_parent.h"
#include "item/s_item.h"
#include "ability/s_ability.h"
#include "buff/s_buff.h"

#define MAX_TOOLTIP_ALPHA 0.9f
#define MAX_TOOLTIP_LINES 10

typedef struct InteractingEnt
{
    int    id;
    int    movingId;
    World *world;
} InteractingEnt;

typedef void (*TooltipDrawFunc)(World *, int);

static void Tooltip_Card_Draw(World *, int);

static const TooltipDrawFunc tooltip_funcs[TOOLTIPKIND_COUNT] = {
    [TOOLTIPKIND_CARD] = Tooltip_Card_Draw,
};

static float          tooltipAlpha;
static InteractingEnt interactingEnt;
static int            topmost_required = BITC(HAS_INTERACT);

static void Interact_Tick(World *world, double dt, double time)
{
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!WHas(world, id, BITC(HAS_INTERACT)))
            continue;
        CompInteract *interact = &world->interacts[id];
        if (interact->state & INTERACT_CLICKED)
        {
            if (interact->state & INTERACT_TOGGLEABLE)
                interact->state ^= INTERACT_TOGGLED;

            if (interact->onClick.callbackFunc)
                interact->onClick.callbackFunc(interact->state, interact->onClick.callbackData);
        }
    }
}

static void Interact_Step(World *world, double dt, double time)
{
    float       fdt       = (float)dt;
    const float stiffness = 80.0f;
    float       alpha     = 1.0f - expf(-stiffness * fdt);

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!WHas(world, id, BITC(HAS_INTERACT)))
            continue;
        CompXform    *xform    = &world->xforms[id];
        CompInteract *interact = &world->interacts[id];
        if (!(interact->state & INTERACT_DRAGGING))
            continue;
        if (WHasB(world, id, HAS_BODY2))
        {
            vec2s grabPos =
                glms_vec2_add((vec2s){xform->pos.x, xform->pos.y}, (vec2s){interact->offset.x, interact->offset.y});
            vec2s targetPos = glms_vec2_sub((vec2s){interact->targetPos.x, interact->targetPos.y}, grabPos);
            Sol_Body2d_SetVel(world, id, glms_vec2_scale(targetPos, alpha));
        }
        else if (WHasB(world, id, HAS_BODY3))
        {
            vec3s grabPos   = vecAdd(xform->pos, interact->offset);
            vec3s targetPos = vecSub(interact->targetPos, grabPos);
            Sol_Physx_SetVel(world, id, glms_vec3_scale(targetPos, alpha));
        }
    }
}

void Sol_Interact_Init(World *world)
{
    world->interacts = calloc(MAX_ENTS, sizeof(CompInteract));
    world->tooltips  = calloc(MAX_ENTS, sizeof(CompTooltip));

    WAddTick(world) = Interact_Tick;
    WAddStep(world) = Interact_Step;
}

CompInteract *Sol_Interact_Add(World *world, int id)
{
    CompInteract  new      = {0};
    CompInteract *interact = &world->interacts[id];
    *interact              = new;
    WAddComp(world, id, HAS_INTERACT);
    return interact;
}

CompTooltip *Sol_Tooltip_Add(World *world, int id, TooltipKind kind)
{
    CompTooltip  new     = {0};
    CompTooltip *tooltip = &world->tooltips[id];
    *tooltip             = new;
    WAddComp(world, id, HAS_TOOLTIP);
    return tooltip;
}

void Sol_Interact_Set(World *world, int id, CompInteract desc)
{
    CompInteract *interact = &world->interacts[id];
    *interact              = desc;
    WAddComp(world, id, HAS_INTERACT);
}

void Sol_Tooltip_Update(double dt, SolUserHit user_hit)
{
    const float stiffness = 5.0f;
    float       alpha     = 1.0f - expf(-stiffness * dt);
    if (user_hit.hoverId && user_hit.hoverWorld)
    {
        int    id    = user_hit.hoverId;
        World *world = user_hit.hoverWorld;
        if ((world->masks[id] & BITC(HAS_TOOLTIP)))
        {
            tooltipAlpha = Sol_Math_Lerp(tooltipAlpha, MAX_TOOLTIP_ALPHA, alpha);
            return;
        }
    }
    tooltipAlpha = 0;
}

void Sol_Tooltip_Draw(double dt, SolUserHit user_hit)
{
    World *world = user_hit.hoverWorld;
    int    id    = user_hit.hoverId;
    if (tooltipAlpha <= 0.0f || !world || id < 1 || user_hit.hoverId == user_hit.focusId)
        return;
    CompTooltip *tooltip = &world->tooltips[id];
    tooltip_funcs[tooltip->kind](world, id);
}

InteractState Sol_Interact_GetState(World *world, int id)
{
    // Self first
    if (world->masks[id] & BITC(HAS_INTERACT))
        return world->interacts[id].state;

    // Fall back to parent
    if (world->masks[id] & BITC(HAS_PARENT))
    {
        int parentId = world->parents[id].parentId;
        if (world->masks[parentId] & BITC(HAS_INTERACT))
            return world->interacts[parentId].state;
    }

    // Neither — no state
    return 0;
}

void Sol_Interact_AddState(World *world, int id, InteractState state)
{
    CompInteract *interact = &world->interacts[id];
    interact->state |= state;
}

void Sol_Interact_ClearState(World *world, int id, InteractState state)
{
    CompInteract *interact = &world->interacts[id];
    interact->state &= ~state;
}

bool Sol_Interact_GetToggle(World *world, int id)
{
    CompInteract *interact = &world->interacts[id];
    return interact->state & INTERACT_TOGGLED;
}

int Sol_Interact_GetTopmost(World *world)
{
    int topZ     = -1;
    int winnerId = -1;

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (id == 0)
            continue;
        if (!WHas(world, id, topmost_required))
            continue;
        if (WHas(world, id, BITC(HAS_BODY2)))
        {
            vec4s bounds = {
                Sol_Xform_GetPos(world, id).x,
                Sol_Xform_GetPos(world, id).y,
                Sol_Body2d_GetDims(world, id).x,
                Sol_Body2d_GetDims(world, id).y,
            };
            if (Sol_Check_2d_Collision(Sol_Input_GetMouseUI(), bounds))
            {
                int z = world->view2d[id].zindex;
                if (z > topZ)
                {
                    topZ     = z;
                    winnerId = id;
                }
            }
        }
    }
    if (winnerId != -1)
        return winnerId;
    if (world->systemBits & BITC(WORLD_SYS_PHYSX))
    {
        SolRayResult result =
            Sol_ScreenRaycast(world, Sol_Input_GetMouse().x, Sol_Input_GetMouse().y, (SolRay){.dist = 15.0f});
        if (result.hit && WHas(world, result.entId, topmost_required))
        {
            return result.entId;
        }
    }
    return -1;
}

void Sol_Interact_DragEntityTo(World *world, int id, vec3s targetPos)
{
    CompInteract *interact = &world->interacts[id];
    if (!(interact->state & INTERACT_DRAGGING))
    {
        interact->state |= INTERACT_DRAGGING;
        interact->offset = vecSub(targetPos, Sol_Xform_GetPos(world, id));

        if (WHas(world, id, BITC(HAS_PARENT)))
        {
            Sol_Parent_SetActive(world, id, false);
        }
    }

    interact->targetPos = targetPos;
}

void Sol_Interact_EndDrag(World *world, int id)
{
    CompInteract *interact = &world->interacts[id];
    interact->state &= ~INTERACT_DRAGGING;

    // Handle parenting / drop logic on release
    if (WHas(world, id, BITC(HAS_BODY2)) && WHas(world, id, BITC(HAS_PARENT)))
    {
        CompBody2d *body = &world->body2d[id];
        if (body->overlapCount > 0)
        {
            Sol_Parent_SetActive(world, id, true);
            Sol_Parent_SetWithOffset(world, id, body->overlapping[0]);
        }
    }
}

static void Render_Tooltip_Line(const char *str, float centerX, float y, float size)
{
    float       width = Sol_MeasureText(str, UISCALE(size), SOL_FONT_ICE);
    SolFontDesc desc  = {
        .color = {0.0f, 1.0f, 0.0f, tooltipAlpha / MAX_TOOLTIP_ALPHA},
        .size  = UISCALE(size),
        .str   = str,
        .x     = centerX - width * 0.5f,
        .y     = y,
    };
    Sol_Render_DrawText2D(desc);
}

static void Tooltip_Card_Draw(World *world, int id)
{
    if (!(world->masks[id] & BITC(HAS_TOOLTIP)))
        return;

    CompTooltip  *tooltip = &world->tooltips[id];
    Item         *item    = &world->items[id].item;
    AbilityConfig cfg     = ability_config[item->ability][item->rarity];

    // --- STAGE 1: COMPUTE STRINGS & MEASURE WIDTHS ---
    // Keep a local stack array of the lines we need to draw
    char lines[MAX_TOOLTIP_LINES][64];
    int  lineCount = 0;

    // Track the raw width maximums
    float maxWidth = 0.0f;

    // 1. Measure and buffer Header
    const char *headerText  = Sol_Ability_GetNameString(item->ability);
    float       headerSize  = 15.0f;
    float       headerWidth = Sol_MeasureText(headerText, UISCALE(headerSize), SOL_FONT_ICE);
    if (headerWidth > maxWidth)
        maxWidth = headerWidth;

    // 2. Buffer & Measure body lines
    float bodyTextSize = 10.0f;

    if (cfg.cooldown > 0.0f && lineCount < MAX_TOOLTIP_LINES)
    {
        snprintf(lines[lineCount], sizeof(lines[lineCount]), "Cooldown: %.1fs", cfg.cooldown);
        float w = Sol_MeasureText(lines[lineCount], UISCALE(bodyTextSize), SOL_FONT_ICE);
        if (w > maxWidth)
            maxWidth = w;
        lineCount++;
    }
    if (cfg.duration > 0.0f && lineCount < MAX_TOOLTIP_LINES)
    {
        snprintf(lines[lineCount], sizeof(lines[lineCount]), "Duration: %.1fs", cfg.duration);
        float w = Sol_MeasureText(lines[lineCount], UISCALE(bodyTextSize), SOL_FONT_ICE);
        if (w > maxWidth)
            maxWidth = w;
        lineCount++;
    }

    float totalDamage = cfg.damage + item->bonusDamage;
    if (totalDamage > 0 && lineCount < MAX_TOOLTIP_LINES)
    {
        if (item->bonusDamage > 0)
        {
            snprintf(lines[lineCount], sizeof(lines[lineCount]), "Damage: %.0f (+%.0f)", totalDamage,
                     item->bonusDamage);
        }
        else
        {
            snprintf(lines[lineCount], sizeof(lines[lineCount]), "Damage: %.0f", totalDamage);
        }
        float w = Sol_MeasureText(lines[lineCount], UISCALE(bodyTextSize), SOL_FONT_ICE);
        if (w > maxWidth)
            maxWidth = w;
        lineCount++;
    }

    u8 totalBuffs = cfg.buffMask | item->bonusBuffs;
    if ((totalBuffs & BITC(BUFFKIND_FIRE)) && lineCount < MAX_TOOLTIP_LINES)
    {
        snprintf(lines[lineCount], sizeof(lines[lineCount]), "Ignite");
        float w = Sol_MeasureText(lines[lineCount], UISCALE(bodyTextSize), SOL_FONT_ICE);
        if (w > maxWidth)
            maxWidth = w;
        lineCount++;
    }
    if ((totalBuffs & BITC(BUFFKIND_STUN)) && lineCount < MAX_TOOLTIP_LINES)
    {
        snprintf(lines[lineCount], sizeof(lines[lineCount]), "Stun");
        float w = Sol_MeasureText(lines[lineCount], UISCALE(bodyTextSize), SOL_FONT_ICE);
        if (w > maxWidth)
            maxWidth = w;
        lineCount++;
    }

    u32 totalEffects = cfg.effectMask | item->bonusEffects;
    if ((totalEffects & (EFFECTMASK_KNOCKBACK | EFFECTMASK_KNOCKBACK_STRONG)) && lineCount < MAX_TOOLTIP_LINES)
    {
        snprintf(lines[lineCount], sizeof(lines[lineCount]), "Knockback");
        float w = Sol_MeasureText(lines[lineCount], UISCALE(bodyTextSize), SOL_FONT_ICE);
        if (w > maxWidth)
            maxWidth = w;
        lineCount++;
    }
    if ((totalEffects & EFFECTMASK_KNOCKUP) && lineCount < MAX_TOOLTIP_LINES)
    {
        snprintf(lines[lineCount], sizeof(lines[lineCount]), "Knockup");
        float w = Sol_MeasureText(lines[lineCount], UISCALE(bodyTextSize), SOL_FONT_ICE);
        if (w > maxWidth)
            maxWidth = w;
        lineCount++;
    }
    if ((totalEffects & EFFECTMASK_REFLECTPROJECTILE) && lineCount < MAX_TOOLTIP_LINES)
    {
        snprintf(lines[lineCount], sizeof(lines[lineCount]), "Reflect");
        float w = Sol_MeasureText(lines[lineCount], UISCALE(bodyTextSize), SOL_FONT_ICE);
        if (w > maxWidth)
            maxWidth = w;
        lineCount++;
    }
    if ((totalEffects & EFFECTMASK_CHAINLIGHTNING) && lineCount < MAX_TOOLTIP_LINES)
    {
        snprintf(lines[lineCount], sizeof(lines[lineCount]), "Chain Lightning");
        float w = Sol_MeasureText(lines[lineCount], UISCALE(bodyTextSize), SOL_FONT_ICE);
        if (w > maxWidth)
            maxWidth = w;
        lineCount++;
    }
    if ((totalEffects & EFFECTMASK_HEALONHIT) && lineCount < MAX_TOOLTIP_LINES)
    {
        snprintf(lines[lineCount], sizeof(lines[lineCount]), "Heal on Hit");
        float w = Sol_MeasureText(lines[lineCount], UISCALE(bodyTextSize), SOL_FONT_ICE);
        if (w > maxWidth)
            maxWidth = w;
        lineCount++;
    }

    // --- STAGE 2: CALCULATE FINAL DYNAMIC DIMS ---
    float ySpacing = UISCALE(12.0f);
    float paddingX = UISCALE(16.0f); // Horizontal internal safe space padding
    float paddingY = UISCALE(16.0f); // Vertical internal safe space padding

    vec2s dims = {.x = maxWidth + paddingX * 2.0f, .y = UISCALE(headerSize) + (lineCount * ySpacing) + paddingY * 2.0f};

    // Keep the box centered or offset cleanly near mouse coordinate profiles
    vec2s pos    = {(float)Sol_Input_GetMouse().x, (float)Sol_Input_GetMouse().y - dims.y};
    vec2s center = {pos.x + dims.x * 0.5f, pos.y + dims.y * 0.5f};

    // --- STAGE 3: DRAW BACKDROP GEOMETRY FIRST ---
    RectSSBO *bg = Sol_Render_GetNext_Rect();
    bg->color    = (vec4s){0.05f, 0.0f, 0.1f, tooltipAlpha};
    bg->dims     = (vec4s){dims.x, dims.y, 0, 1.0f};
    bg->pos      = (vec4s){pos.x, pos.y, 0, 1.0f};

    RectSSBO *border  = Sol_Render_GetNext_Rect();
    border->color     = (vec4s){0.0f, 0.0f, 0.02f, tooltipAlpha};
    border->dims      = (vec4s){dims.x, dims.y, 0, 1.0f};
    border->pos       = (vec4s){pos.x, pos.y, 0, 1.0f};
    border->uv        = (vec4s){0, 0, 1, 1};
    border->textureID = SOL_TEXTURE_CLOUD1;

    RectSSBO *border2  = Sol_Render_GetNext_Rect();
    border2->color     = (vec4s){0.0f, 0.0f, 0.2f, tooltipAlpha};
    border2->dims      = (vec4s){dims.x, dims.y, 0, 1.0f};
    border2->pos       = (vec4s){pos.x, pos.y, 0, 1.0f};
    border2->uv        = (vec4s){0, 0, 1, 1};
    border2->textureID = SOL_TEXTURE_SWIRLFRAME;

    RectSSBO *border3  = Sol_Render_GetNext_Rect();
    border3->color     = (vec4s){1.0f, 1.0f, 1.0f, 0.15f};
    border3->dims      = (vec4s){dims.x, dims.y, 0.0f, 1.0f};
    border3->pos       = (vec4s){pos.x, pos.y, 0, 1.0f};
    border3->uv        = (vec4s){0, 0, 1, 1};
    border3->textureID = SOL_TEXTURE_BORDER;

    // --- STAGE 4: RENDER THE TEXT ON TOP ---
    // Start layout downward from top padding limits
    float currentY = pos.y + paddingY + (UISCALE(headerSize) * 0.5f);

    SolFontDesc headerDesc = {
        .color = {1.0f, 0, 0, tooltipAlpha / MAX_TOOLTIP_ALPHA},
        .size  = UISCALE(headerSize),
        .str   = headerText,
        .x     = center.x - headerWidth * 0.5f,
        .y     = currentY,
    };
    Sol_Render_DrawText2D(headerDesc);

    // Step below header bounds
    currentY += UISCALE(headerSize);

    // Flush out the remaining tracked strings
    for (int i = 0; i < lineCount; i++)
    {
        Render_Tooltip_Line(lines[i], center.x, currentY, bodyTextSize);
        currentY += ySpacing;
    }
}