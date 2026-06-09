/*
 * File: s_interact.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Interact!
 */
#include "sol_core.h"

#define MAX_TOOLTIP_ALPHA 0.9f
typedef void (*TooltipDrawFunc)(World *, int);
static void                  SetMoving(World *world, CompInteract *interact, int id);
static void                  Tooltip_Card_Draw(World *, int);
static const TooltipDrawFunc tooltip_funcs[TOOLTIPKIND_COUNT] = {
    [TOOLTIPKIND_CARD] = Tooltip_Card_Draw,
};

static float          tooltipAlpha;
static InteractingEnt interactingEnt;

void Sol_Interact_Init(World *world)
{
    world->interacts = calloc(MAX_ENTS, sizeof(CompInteract));
    world->tooltips  = calloc(MAX_ENTS, sizeof(CompTooltip));
}

CompInteract *Sol_Interact_Add(World *world, int id)
{
    CompInteract interact = {0};
    world->interacts[id]  = interact;
    world->masks[id] |= HAS_INTERACT;
    return &world->interacts[id];
}

CompTooltip *Sol_Tooltip_Add(World *world, int id, TooltipKind kind)
{
    world->masks[id] |= HAS_TOOLTIP;
    return &world->tooltips[id];
}

void Sol_Interact_Set(World *world, int id, CompInteract desc)
{
    world->interacts[id] = desc;
    world->masks[id] |= HAS_INTERACT;
}

static int FindTopMost(World *world)
{
    int topZ     = INT_MIN;
    int winnerId = -1;
    int required = HAS_INTERACT | HAS_BODY2;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        vec4s bounds = {
            Sol_Xform_GetPos(world, id).x,
            Sol_Xform_GetPos(world, id).y,
            Sol_Body2d_GetDims(world, id).x,
            Sol_Body2d_GetDims(world, id).y,
        };
        if (Sol_Check_2d_Collision(Sol_Input_GetMouseUI(), bounds))
        {
            int z = world->body2d[id].zindex;
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

        if (mWorld->masks[mId] & HAS_PARENT)
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
            if (interactingEnt.world->masks[oldId] & HAS_INTERACT)
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
                if (glms_vec2_distance(interact->pressPos, Sol_Input_GetMouseUI()) > 2.0f)
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
        if ((world->masks[id] & HAS_TOOLTIP))
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
    if (world->masks[id] & HAS_INTERACT)
        return world->interacts[id].state;

    // Fall back to parent
    if (world->masks[id] & HAS_PARENT)
    {
        int parentId = world->parents[id].parentId;
        if (world->masks[parentId] & HAS_INTERACT)
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
    if (world->masks[id] & HAS_PARENT)
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
    if (!(world->masks[id] & HAS_TOOLTIP))
        return;
    CompTooltip  *tooltip = &world->tooltips[id];
    CompItem     *item    = &world->items[id];
    AbilityConfig cfg     = ability_config[item->ability][item->rarity];

    vec2s dims   = {UISCALE(100.0f), UISCALE(100.0f)};
    vec2s pos    = {Sol_Input_GetMouse().x, Sol_Input_GetMouse().y - dims.y};
    vec2s center = {pos.x + dims.y * 0.5f, pos.y + dims.x * 0.5f};

    RectSSBO *bg       = Sol_Render_GetNext_Rect();
    bg->color          = (vec4s){0.05f, 0.0f, 0.1f, tooltipAlpha};
    bg->dims           = (vec4s){dims.x, dims.y, 0, 1.0f};
    bg->pos            = (vec4s){pos.x, pos.y, 0, 1.0f};
    RectSSBO *border   = Sol_Render_GetNext_Rect();
    border->color      = (vec4s){0.0f, 0.4f, 0.2f, tooltipAlpha};
    border->dims       = (vec4s){dims.x, dims.y, 0, 1.0f};
    border->pos        = (vec4s){pos.x, pos.y, 0, 1.0f};
    border->uv         = (vec4s){0, 0, 1, 1};
    border->textureID  = SOL_TEXTURE_CLOUD1;
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

    float       headerSize  = 15.0f;
    const char *headerText  = Sol_Ability_GetNameString(item->ability);
    float       headerWidth = Sol_MeasureText(headerText, UISCALE(headerSize), SOL_FONT_ICE);
    SolFontDesc headerDesc  = {
        .color = {1.0f, 0, 0, tooltipAlpha / MAX_TOOLTIP_ALPHA},
        .size  = UISCALE(headerSize),
        .str   = headerText,
        .x     = center.x - headerWidth * 0.5f,
        .y     = center.y - dims.y * 0.3f,
    };
    Sol_Render_DrawText2D(headerDesc);

    // float bodyTextSize = 12.0f;
    // float yStart       = center.y - dims.y * 0.15f;
    // float ySpacing     = dims.y * 0.15f;
    // for (int i = 0; i < tooltip->as.card.effectCount; i++)
    // {
    //     char  *effectText = tooltip->as.card.effect[i];
    //     size_t length     = strlen(effectText);

    //     float       bodyTextWidth = Sol_MeasureText(effectText, UISCALE(bodyTextSize), SOL_FONT_ICE);
    //     SolFontDesc cooldownDesc  = {
    //         .color = {0.0f, 1.0f, 0.0f, tooltipAlpha / MAX_TOOLTIP_ALPHA},
    //         .size  = UISCALE(bodyTextSize),
    //         .str   = effectText,
    //         .x     = center.x - bodyTextWidth * 0.5f,
    //         .y     = yStart + ySpacing * i,
    //     };
    //     Sol_Render_DrawText2D(cooldownDesc);
    // }
    float bodyTextSize = 10.0f;
    float yStart       = center.y - dims.y * 0.15f;
    float ySpacing     = dims.y * 0.12f;
    int   lineCount    = 0;
    char  tempBuffer[32];

    // Render Cooldown Line
    if (cfg.cooldown > 0.0f)
    {
        snprintf(tempBuffer, sizeof(tempBuffer), "Cooldown: %.1fs", cfg.cooldown);
        Render_Tooltip_Line(tempBuffer, center.x, yStart + (ySpacing * lineCount++), bodyTextSize);
    }
    if (cfg.duration > 0.0f)
    {
        snprintf(tempBuffer, sizeof(tempBuffer), "Duration: %.1fs", cfg.duration);
        Render_Tooltip_Line(tempBuffer, center.x, yStart + (ySpacing * lineCount++), bodyTextSize);
    }

    // Render Damage Line (incorporating dynamic procedural modifications)
    u32 totalDamage = cfg.damage + item->bonusDamage;
    if (totalDamage > 0)
    {
        if (item->bonusDamage > 0)
        {
            snprintf(tempBuffer, sizeof(tempBuffer), "Damage: %d (+%d)", totalDamage, item->bonusDamage);
        }
        else
        {
            snprintf(tempBuffer, sizeof(tempBuffer), "Damage: %d", totalDamage);
        }
        Render_Tooltip_Line(tempBuffer, center.x, yStart + (ySpacing * lineCount++), bodyTextSize);
    }
    u8 totalBuffs = cfg.buffMask | item->bonusBuffs;
    if (totalBuffs & BUFFMASK_FIRE)
        Render_Tooltip_Line("Ignite", center.x, yStart + (ySpacing * lineCount++), bodyTextSize);

    u32 totalEffects = cfg.effectMask | item->bonusEffects;
    if (totalEffects & (EFFECTMASK_KNOCKBACK | EFFECTMASK_KNOCKBACK_STRONG))
        Render_Tooltip_Line("Knockback", center.x, yStart + (ySpacing * lineCount++), bodyTextSize);

    if (totalEffects & EFFECTMASK_KNOCKUP)
        Render_Tooltip_Line("Knockup", center.x, yStart + (ySpacing * lineCount++), bodyTextSize);
}
