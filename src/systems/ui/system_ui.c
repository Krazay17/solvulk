#include "sol_core.h"

void Sol_System_UI_Draw(World *world, double dt, double time)
{
    int required = HAS_XFORM | HAS_SHAPE | HAS_UI_ELEMENT;
    float fdt = (float)dt;

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required) continue;

        CompXform *xform = &world->xforms[id];
        CompShape *shape = &world->shapes[id];
        CompUiElement *ui = &world->uiElements[id];
        
        // We still check HAS_INTERACT to drive animations
        bool isHovered = false;
        if (world->masks[id] & HAS_INTERACT) {
            isHovered = world->interactables[id].isHovered;
            
            // Update animations right here in the draw system for now
            if (isHovered) ui->hoverAnim = fminf(ui->hoverAnim + fdt * 12.0f, 1.0f);
            else ui->hoverAnim = fmaxf(ui->hoverAnim - fdt * 8.0f, 0.0f);
            
            if (world->interactables[id].isClicked) ui->clickAnim = 1.0f;
            ui->clickAnim = fmaxf(ui->clickAnim - fdt * 5.0f, 0.0f);
        }

        // --- DRAWING LOGIC ---
        SolRect rect = {xform->pos.x, xform->pos.y, shape->width, shape->height};
        
        // Background with hover/click tint
        SolColor drawCol = Sol_Color_Lerp(ui->baseColor, (SolColor){255,255,255,255}, ui->hoverAnim * 0.2f + ui->clickAnim * 0.5f);
        Sol_Draw_Rectangle(rect, drawCol, 0);

        // Border
        if (ui->borderThickness > 0 || isHovered) {
            float thickness = ui->borderThickness + (ui->clickAnim * 4.0f);
            Sol_Draw_Rectangle(rect, ui->borderColor, thickness);
        }

        // Text
        if (ui->text[0] != '\0') {
            float tx = rect.x + (rect.w * 0.5f) - (ui->textWidth * 0.5f);
            float ty = rect.y + (rect.h * 0.5f) + (ui->fontSize * 0.3f);
            Sol_Draw_Text(ui->text, tx, ty, ui->fontSize, ui->textColor);
        }
    }
}