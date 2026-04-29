#include "sol_core.h"

CompInteractable *Sol_Interact_Add(World *world, int id)
{
    world->masks[id] |= HAS_INTERACT;
    return &world->interactables[id];
}

void Sol_System_Interact_Ui(World *world, double dt, double time)
{
    int      required = HAS_INTERACT | HAS_XFORM | HAS_SHAPE;
    SolMouse mouse    = Sol_Input_GetMouse();

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompInteractable *interact = &world->interactables[id];
            CompXform        *xform    = &world->xforms[id];
            CompShape        *shape    = &world->shapes[id];

            bool wasPressed     = interact->isPressed;
            interact->isClicked = false;

            interact->isHovered = Sol_Check_2d_Collision(
                (vec2s){mouse.x, mouse.y}, (vec4s){xform->pos.x, xform->pos.y, shape->width, shape->height});

            if (interact->isHovered)
            {
                if (mouse.buttons[SOL_MOUSE_LEFT])
                {
                    interact->isPressed = true;
                    if (interact->onHold)
                    {
                        if (interact->callback.callbackData)
                            interact->callback.callbackFunc(interact->callback.callbackData);
                        else if (interact->callback.callbackFunc)
                            interact->callback.callbackFunc(NULL);
                    }
                }
                else if (wasPressed)
                {
                    interact->isClicked = true;
                    interact->isPressed = false;
                    if (interact->callback.callbackData)
                        interact->callback.callbackFunc(interact->callback.callbackData);
                    else if (interact->callback.callbackFunc)
                        interact->callback.callbackFunc(NULL);
                }
            }
            else
            {
                interact->isPressed = false;
            }
        }
    }
}