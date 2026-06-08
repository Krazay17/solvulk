/*
 * File: s_interact.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Interact!
 */
#include "sol_core.h"

static void SetMoving(World *world, CompInteract *interact, int id);

static bool           showTooltip;
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
    Sol_Tooltip_Update();
}

void Sol_Tooltip_Update()
{
    if (interactingEnt.id && interactingEnt.world)
    {
        int    id    = interactingEnt.id;
        World *world = interactingEnt.world;
        if ((world->masks[id] & HAS_TOOLTIP))
        {
            showTooltip = true;
            return;
        }
    }
    showTooltip = false;
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
    if (!showTooltip)
        return;
    World       *world   = interactingEnt.world;
    int          id      = interactingEnt.id;
    CompTooltip *tooltip = &world->tooltips[id];

    vec4s pos  = {Sol_Input_GetMouseUI().x, Sol_Input_GetMouseUI().y - 200.0f, 0, 1.0f};
    vec2s dims = {200.0f, 200.0f};

    RectSSBO *rectSSBO = Sol_Render_GetNext_Rect();
    rectSSBO->color    = (vec4s){1.0f, 1.0f, 1.0f, 1.0f};
    rectSSBO->dims     = (vec4s){200.0f, 200.0f, 0, 1.0f};
    rectSSBO->pos      = (vec4s){Sol_Input_GetMouseUI().x, Sol_Input_GetMouseUI().y - 200.0f, 0, 1.0f};
    FontSSBO *fontSSBO = Sol_Render_GetNext_Font();
    fontSSBO->color    = (vec4s){1.0f, 1.0f, 0.0f, 1.0f};

    fontSSBO->pos = pos;
}