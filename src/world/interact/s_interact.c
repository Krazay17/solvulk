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
#include "physx/s_body2d.h"
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

static void SetMoving(World *world, CompInteract *interact, int id);
static void Tooltip_Card_Draw(World *, int);

static const TooltipDrawFunc tooltip_funcs[TOOLTIPKIND_COUNT] = {
    [TOOLTIPKIND_CARD] = Tooltip_Card_Draw,
};

static float          tooltipAlpha;
static InteractingEnt interactingEnt;

void Sol_Interact_Init(World *world)
{
    // world->components[HAS_INTERACT] = calloc(MAX_ENTS, sizeof(CompInteract));
    // world->components[HAS_TOOLTIP]  = calloc(MAX_ENTS, sizeof(CompTooltip));
    world->interacts = calloc(MAX_ENTS, sizeof(CompInteract));
    world->tooltips  = calloc(MAX_ENTS, sizeof(CompTooltip));
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

static int topmost_required = BITC(HAS_INTERACT) | BITC(HAS_BODY2);
static int FindTopMost(World *world)
{
    CompBody2d *body2ds = &world->body2d;

    int topZ     = INT_MIN;
    int winnerId = -1;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!WHas(world, id, topmost_required))
            continue;
        vec4s bounds = {
            Sol_Xform_GetPos(world, id).x,
            Sol_Xform_GetPos(world, id).y,
            Sol_Body2d_GetDims(world, id).x,
            Sol_Body2d_GetDims(world, id).y,
        };
        if (Sol_Check_2d_Collision(Sol_Input_GetMouseUI(), bounds))
        {
            int z = body2ds[id].zindex;
            if (z > topZ)
            {
                topZ     = z;
                winnerId = id;
            }
        }
    }
    return winnerId;
}

void Sol_Interact_Update(World **worlds, int count)
{
    SolMouse mouse = Sol_Input_GetMouse();
    // =========================================================================
    // STATE 1: ACTIVE DRAGGING LAYER
    // =========================================================================
    if (interactingEnt.movingId != 0)
    {
        // Continuous Dragging Phase: Exit early and let your move system update transforms
        if (mouse.buttons[SOL_MOUSE_LEFT])
        {
            return;
        }

        // Drop & Cleanup Phase: Executes the exact frame the mouse button is released
        World        *mWorld   = interactingEnt.world;
        int           mId      = interactingEnt.movingId;
        CompInteract *interact = &mWorld->interacts[mId];
        CompBody2d   *body     = &mWorld->body2d[mId];

        interact->state &= ~INTERACT_MOVING;

        // MOVING GUARD: Clear the pressed state flag right here!
        // This ensures State 2 cannot trigger a click event on the drop frame.
        interact->state &= ~INTERACT_PRESSED;

        if (mWorld->masks[mId] & BITC(HAS_PARENT))
        {
            if (body->overlapCount)
            {
                Sol_Parent_SetActive(mWorld, mId, true);
                Sol_Parent_SetWithOffset(mWorld, mId, body->overlapping[0]);
            }
        }

        // Terminate dragging identifiers completely
        interactingEnt.movingId = 0;

        // Let the state fall through to scan whatever is underneath the mouse on drop frame
    }

    // =========================================================================
    // STATE 2: PASSIVE SCAN & CLICK EVALUATION LAYER
    // =========================================================================
    int    winner      = -1;
    World *winnerWorld = NULL;

    // 1. Scan worlds backwards from front-most screen projection (UI down to Gameplay)
    for (int w = count - 1; w >= 0; w--)
    {
        World *world = worlds[w];
        if (!world->doesSimulate)
            continue;

        int foundId = FindTopMost(world);
        if (foundId != -1)
        {
            winner      = foundId;
            winnerWorld = world;
            break;
        }
    }

    // 2. Hover Transitions (O(1) State Change Flags)
    if (winner != interactingEnt.id || winnerWorld != interactingEnt.world)
    {
        // Wipe old focus targets cleanly
        if (interactingEnt.world && interactingEnt.id != -1)
        {
            int oldId = interactingEnt.id;
            if (interactingEnt.world->masks[oldId] & BITC(HAS_INTERACT))
            {
                CompInteract *oldInteract = &interactingEnt.world->interacts[oldId];
                oldInteract->state &= ~INTERACT_HOVERED;
                oldInteract->state &= ~INTERACT_PRESSED;
                oldInteract->state &= ~INTERACT_CLICKED;
            }
        }

        // Bind global tracker to our current physical layer target
        interactingEnt.id    = winner;
        interactingEnt.world = winnerWorld;
    }

    // 3. Action Selection & Click Processing
    if (interactingEnt.id != -1 && interactingEnt.world != NULL)
    {
        int           id       = interactingEnt.id;
        World        *world    = interactingEnt.world;
        CompInteract *interact = &world->interacts[id];

        // This will now correctly read false if the item was just dropped!
        bool wasPressed = interact->state & INTERACT_PRESSED;

        // Reset single-frame transient execution triggers
        interact->state &= ~INTERACT_PRESSED;
        interact->state &= ~INTERACT_CLICKED;
        interact->state |= INTERACT_HOVERED;

        // Button Down Evaluator Pass
        if (mouse.buttons[SOL_MOUSE_LEFT])
        {
            interact->state |= INTERACT_PRESSED;

            if (wasPressed)
            {
                // Break deadzone bounds threshold to turn click state into active move state
                if (glms_vec2_distance(interact->pressPos, Sol_Input_GetMouseUI()) > 1.0f)
                {
                    SetMoving(world, interact, id);
                }
            }
            else
            {
                // Frame exact click touchdown snapshot
                interact->pressPos = Sol_Input_GetMouseUI();
            }

            if (interact->onHold.callbackFunc)
                interact->onHold.callbackFunc(interact->state, interact->onHold.callbackData);
        }
        // Button Released Evaluator Pass (Clean standard item clicking)
        else if (wasPressed)
        {
            interact->state |= INTERACT_CLICKED;

            if (interact->state & INTERACT_TOGGLEABLE)
                interact->state ^= INTERACT_TOGGLED;

            if (interact->onClick.callbackFunc)
                interact->onClick.callbackFunc(interact->state, interact->onClick.callbackData);
        }
    }
    else
    {
        interactingEnt.id    = 0;
        interactingEnt.world = NULL;
    }
    Sol_Tooltip_Update(SOL_TIMESTEP);
}

void Sol_Tooltip_Update(double dt)
{
    const float stiffness = 5.0f;
    float       alpha     = 1.0f - expf(-stiffness * dt);
    if (interactingEnt.id && interactingEnt.world)
    {
        int    id    = interactingEnt.id;
        World *world = interactingEnt.world;
        if ((world->masks[id] & BITC(HAS_TOOLTIP)))
        {
            tooltipAlpha = Sol_Math_Lerp(tooltipAlpha, MAX_TOOLTIP_ALPHA, alpha);
            return;
        }
    }
    tooltipAlpha = 0;
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

bool Sol_Interact_GetToggle(World *world, int id)
{
    CompInteract *interact = &world->interacts[id];
    return interact->state & INTERACT_TOGGLED;
}

static void SetMoving(World *world, CompInteract *interact, int id)
{
    interact->state |= INTERACT_MOVING;
    interactingEnt.movingId = id;
    interact->grabOffset =
        glms_vec2_sub(Sol_Input_GetMouseUI(), (vec2s){world->xforms[id].pos.x, world->xforms[id].pos.y});
    if (world->masks[id] & BITC(HAS_PARENT))
        Sol_Parent_SetActive(world, id, false);
}

void Sol_Tooltip_Draw()
{
    World *world = interactingEnt.world;
    int    id    = interactingEnt.id;
    if (tooltipAlpha <= 0.0f || !world || id < 1 || interactingEnt.id == interactingEnt.movingId)
        return;
    CompTooltip *tooltip = &world->tooltips[id];
    tooltip_funcs[tooltip->kind](world, id);
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