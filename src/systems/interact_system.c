#include "sol_core.h"
#include "world/components.h"
#include "xform/xform.h"

typedef struct CompInteract
{
    InteractState state;
    float         value;
    Callback      onClick;
    Callback      onHold;
} CompInteract;

void Sol_Interact_Init(World *world)
{
    world->interacts = calloc(MAX_ENTS, sizeof(CompInteract));
}

void Sol_Interact_Add(World *world, int id, InteractDesc desc)
{
    CompInteract interact = {0};
    world->interacts[id]  = interact;
    world->masks[id] |= HAS_INTERACT;
}

void System_Interact_Tick(World *world, double dt, double time)
{
    SolMouse     mouse     = Sol_Input_GetMouse();
    int          rayId     = -1;
    SolRayResult screenRay = {0};
    if (!mouse.locked)
        screenRay = Sol_ScreenRaycast(world, mouse.x, mouse.y, (SolRay){.mask = 0b11, .dist = 100.0f});
    if (screenRay.hit && screenRay.entId)
        rayId = screenRay.entId;

    Sol_Debug_Add("SelectedEnt", rayId);

    int required = HAS_INTERACT | HAS_XFORM;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompInteract *interact   = &world->interacts[id];
        CompXform    *xform      = &world->xforms[id];
        bool          wasPressed = interact->state & INTERACT_PRESSED;
        interact->state &= ~INTERACT_PRESSED;
        interact->state &= ~INTERACT_CLICKED;
        interact->state &= ~INTERACT_HOVERED;
        bool collision = false;

        if (world->masks[id] & HAS_BODY3)
        {
            if (rayId == id)
            {
                collision = true;
            }
        }
        else
        {
            vec2s mousePos = (vec2s){mouse.x, mouse.y};
            collision      = Sol_Check_2d_Collision(mousePos, (vec4s){
                                                                  UISCALE(xform->pos.x),
                                                                  UISCALE(xform->pos.y),
                                                                  UISCALE(xform->width),
                                                                  UISCALE(xform->height),
                                                              });
        }
        if (collision)
        {
            interact->state |= INTERACT_HOVERED;
            if (mouse.buttons[SOL_MOUSE_LEFT])
            {
                interact->state |= INTERACT_PRESSED;

                if (interact->onHold.callbackFunc)
                    interact->onHold.callbackFunc(interact->onHold.callbackData);
            }
            else if (wasPressed)
            {
                interact->state |= INTERACT_CLICKED;
                interact->state &= ~INTERACT_PRESSED;
                if (interact->state & INTERACT_ISTOGGLE)
                    interact->state ^= INTERACT_TOGGLED;

                if (interact->onClick.callbackFunc)
                    interact->onClick.callbackFunc(interact->onClick.callbackData);
            }
        }
        else
        {
            interact->state &= ~INTERACT_PRESSED;
        }
    }
}

InteractState Sol_Interact_GetState(World *world, int id)
{
    return world->interacts[id].state;
}