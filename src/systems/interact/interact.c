#include "sol_core.h"

CompInteract *Sol_Interact_Add(World *world, int id)
{
    CompInteract interact = {0};
    world->interacts[id]  = interact;
    world->masks[id] |= HAS_INTERACT;
    return &world->interacts[id];
}

void Interact2d_Tick(World *world, double dt, double time)
{
    int      required = HAS_INTERACT | HAS_XFORM | HAS_SHAPE;
    SolMouse mouse    = Sol_Input_GetMouse();

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompInteract *interact = &world->interacts[id];
            CompXform    *xform    = &world->xforms[id];
            CompShape    *shape    = &world->shapes[id];

            bool wasPressed = interact->states & INTERACT_PRESSED;
            interact->states &= ~INTERACT_PRESSED;
            interact->states &= ~INTERACT_CLICKED;
            interact->states &= ~INTERACT_HOVERED;

            vec2s mousePos = (vec2s){mouse.x, mouse.y};

            if (Sol_Check_2d_Collision(mousePos, (vec4s){xform->pos.x, xform->pos.y, shape->width, shape->height}))
            {
                interact->states |= INTERACT_HOVERED;
                if (mouse.buttons[SOL_MOUSE_LEFT])
                {
                    interact->states |= INTERACT_PRESSED;

                    if (interact->onHold.callbackFunc)
                        interact->onHold.callbackFunc(interact->onHold.callbackData);
                }
                else if (wasPressed)
                {
                    interact->states |= INTERACT_CLICKED;
                    interact->states &= ~INTERACT_PRESSED;
                    if(interact->states & INTERACT_ISTOGGLE)
                    interact->states ^= INTERACT_TOGGLED;

                    if (interact->onClick.callbackFunc)
                        interact->onClick.callbackFunc(interact->onClick.callbackData);
                }
            }
            else
            {
                interact->states &= ~INTERACT_PRESSED;
            }
        }
    }
}

void Interact3d_Tick(World *world, double dt, double time)
{
}