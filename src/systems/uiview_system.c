#include "sol_core.h"
#include "xform/xform.h"

typedef struct CompUiButton
{
    bool isToggle, toggle;
} CompUiButton;

typedef struct CompUiSlider
{
    float value;
    float min, max;
    bool  isDragging;
} CompUiSlider;

typedef struct CompUi
{
    UiKind kind;
    vec4s  baseColor;
    vec4s  textColor;
    float  fontSize;
    char   text[64];
    float  textWidth;

    float hoverAnim;
    float clickAnim;

    float borderThickness;
    vec4s borderColor;
} CompUi;

void Sol_Ui_Init(World *world)
{
    world->uiElements = calloc(MAX_ENTS, sizeof(CompUi));
}

void Sol_Ui_Add(World *world, int id, UiDesc desc)
{
    if (!world || !world->uiElements)
        return;
    world->masks[id] |= HAS_UIVIEW;
    CompUi ui = {
        .kind = desc.kind,
    };
    world->uiElements[id] = ui;
}

void Sol_Ui_Draw(World *world, double dt, double time)
{
    int   required = HAS_XFORM | HAS_SHAPE | HAS_UIVIEW;
    float fdt      = (float)dt;

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompXform *xform = &world->xforms[id];
        CompUi    *view  = &world->uiElements[id];

        bool isHovered = false;
        if (world->masks[id] & HAS_INTERACT)
        {
            if (Sol_Interact_GetState(world, id) & INTERACT_HOVERED)
                view->hoverAnim = fminf(view->hoverAnim + dt * 12.0f, 1.0f);
            else
                view->hoverAnim = fmaxf(view->hoverAnim - dt * 8.0f, 0.0f);
            if (Sol_Interact_GetState(world, id) & INTERACT_CLICKED)
                view->clickAnim = 1.0f;
            view->clickAnim = fmaxf(view->clickAnim - dt * 5.0f, 0.0f);
        }

        // --- DRAWING LOGIC ---
        vec4s rect    = {UISCALE(xform->pos.x), UISCALE(xform->pos.y), UISCALE(xform->width), UISCALE(xform->height)};
        vec4s drawCol = view->baseColor;
        if (Sol_Interact_GetState(world, id) & INTERACT_TOGGLED)
            drawCol = (vec4s){80, 200, 80, 255};
        drawCol = Sol_Color_Lerp(drawCol, (vec4s){255, 255, 255, 255}, view->hoverAnim * 0.2f + view->clickAnim * 0.5f);

        Render_Draw_Rectangle(rect, drawCol, 0);

        // Border
        if (view->borderThickness > 0 || isHovered)
        {
            float thickness = view->borderThickness + (view->clickAnim * 4.0f) + (view->hoverAnim * 2.0f);
            Render_Draw_Rectangle(rect, view->borderColor, thickness);
        }

        // Text
        if (view->text[0] != '\0')
        {
            float       tx       = rect.x + (rect.w * 0.5f) - (UISCALE(view->textWidth) * 0.5f);
            float       ty       = rect.y + (rect.z * 0.5f) + (view->fontSize * 0.3f);
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