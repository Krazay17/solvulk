#include "sol_core.h"

CompInteract *Sol_Interact_Add(World *world, int id)
{
    CompInteract interact = {0};
    world->interacts[id]  = interact;
    world->masks[id] |= HAS_INTERACT;
    return &world->interacts[id];
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
        if ((world->masks[id] & required) == required)
        {
            CompInteract *interact = &world->interacts[id];
            CompXform    *xform    = &world->xforms[id];
            CompShape    *shape    = (world->masks[id] & HAS_SHAPE) ? &world->shapes[id] : NULL;
            CompBody     *body     = (world->masks[id] & HAS_BODY3) ? &world->bodies[id] : NULL;

            bool wasPressed = interact->states & INTERACT_PRESSED;
            interact->states &= ~INTERACT_PRESSED;
            interact->states &= ~INTERACT_CLICKED;
            interact->states &= ~INTERACT_HOVERED;

            vec2s mousePos = (vec2s){mouse.x, mouse.y};

            bool collision = false;
            if (body)
            {
                if (rayId == id)
                {
                    collision = true;
                }
            }
            else if (shape)
                collision = Sol_Check_2d_Collision(mousePos, (vec4s){
                                                                 UISCALE(xform->pos.x),
                                                                 UISCALE(xform->pos.y),
                                                                 UISCALE(shape->width),
                                                                 UISCALE(shape->height),
                                                             });

            if (collision)
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
                    if (interact->states & INTERACT_ISTOGGLE)
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