#include "sol_core.h"

CompUiView *Sol_UiView_Add(World *world, int id)
{
    world->masks[id] |= HAS_UIVIEW;
    return &world->uiElements[id];
}

void Sol_UiView_Draw(World *world, double dt, double time)
{
    int   required = HAS_XFORM | HAS_SHAPE | HAS_UIVIEW;
    float fdt      = (float)dt;

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompXform    *xform    = &world->xforms[id];
        CompShape    *shape    = &world->shapes[id];
        CompUiView   *view     = &world->uiElements[id];
        CompInteract *interact = (world->masks[id] & HAS_INTERACT) ? &world->interacts[id] : NULL;

        bool isHovered = false;
        if (interact)
        {
            if (interact->states & INTERACT_HOVERED)
                view->hoverAnim = fminf(view->hoverAnim + dt * 12.0f, 1.0f);
            else
                view->hoverAnim = fmaxf(view->hoverAnim - dt * 8.0f, 0.0f);
            if (interact->states & INTERACT_CLICKED)
                view->clickAnim = 1.0f;
            view->clickAnim = fmaxf(view->clickAnim - dt * 5.0f, 0.0f);
        }

        // --- DRAWING LOGIC ---
        SolRect rect    = {UISCALE(xform->pos.x), UISCALE(xform->pos.y), UISCALE(shape->width), UISCALE(shape->height)};
        vec4s   drawCol = view->baseColor;
        if (interact->states & INTERACT_TOGGLED)
            drawCol = (vec4s){80, 200, 80, 255};
        drawCol = Sol_Color_Lerp(drawCol, (vec4s){255, 255, 255, 255}, view->hoverAnim * 0.2f + view->clickAnim * 0.5f);

        Sol_Draw_Rectangle(rect, drawCol, 0);

        // Border
        if (view->borderThickness > 0 || isHovered)
        {
            float thickness = view->borderThickness + (view->clickAnim * 4.0f) + (view->hoverAnim * 2.0f);
            Sol_Draw_Rectangle(rect, view->borderColor, thickness);
        }

        // Text
        if (view->text[0] != '\0')
        {
            float       tx       = rect.x + (rect.w * 0.5f) - (UISCALE(view->textWidth) * 0.5f);
            float       ty       = rect.y + (rect.h * 0.5f) + (view->fontSize * 0.3f);
            SolFontDesc fontDesc = {
                .str   = view->text,
                .x     = tx,
                .y     = ty,
                .size  = UISCALE(view->fontSize),
                .color = view->textColor,
                .kind  = SOL_FONT_ICE,
            };
            Sol_Draw_Text(fontDesc);
        }
    }
}