#include "world.h"
#include "font.h"
#include "render/render.h"
#include "sol_core.h"

static void Render_Tooltip_Line(const char *str, float centerX, float y, float size, float alpha)
{
    float width      = Sol_MeasureText(str, UISCALE(size), SOL_FONT_ICE);
    SolFontDesc desc = {
        .color = {0.0f, 1.0f, 0.0f, alpha / MAX_TOOLTIP_ALPHA},
        .size  = UISCALE(size),
        .layer = UILAYER_3,
        .x     = centerX - width * 0.5f,
        .y     = y,
    };
    Sol_Render_DrawText2D(str, desc);
}

static void Item_Draw(World *world, int id, UserInput *input, float alpha)
{
    ScRef *ref = Sol_Comp_Get(world, id, ScRef);
    if (!ref)
        return;
    SolItem *item = Sol_Item_Resolve(world, ref);
    if (!item)
        return;

    // --- STAGE 1: COMPUTE STRINGS & MEASURE WIDTHS ---
    // Keep a local stack array of the lines we need to draw
    char lines[MAX_TOOLTIP_LINES][64];
    int lineCount = 0;

    // Track the raw width maximums
    float maxWidth    = 0.0f;
    AbilityConfig cfg = Sol_Item_GetMods(*item);

    // 1. Measure and buffer Header
    const char *headerText = item_name[item->item_kind];
    float headerSize       = 15.0f;
    float headerWidth      = Sol_MeasureText(headerText, UISCALE(headerSize), SOL_FONT_ICE);
    if (headerWidth > maxWidth)
        maxWidth = headerWidth;

    // 2. Buffer & Measure body lines
    float bodyTextSize = 10.0f;

    if (cfg.cooldown > 0.0f && lineCount < MAX_TOOLTIP_LINES)
    {
        snprintf(lines[lineCount], sizeof(lines[lineCount]), "Cooldown: -%%%.f", cfg.cooldown * 100.0f);
        float w = Sol_MeasureText(lines[lineCount], UISCALE(bodyTextSize), SOL_FONT_ICE);
        if (w > maxWidth)
            maxWidth = w;
        lineCount++;
    }
        if (lineCount < MAX_TOOLTIP_LINES)
    {
        snprintf(lines[lineCount], sizeof(lines[lineCount]), "Uid: %d", item->uid);
        float w = Sol_MeasureText(lines[lineCount], UISCALE(bodyTextSize), SOL_FONT_ICE);
        if (w > maxWidth)
            maxWidth = w;
        lineCount++;
    }

    if (cfg.speed > 0.0f && lineCount < MAX_TOOLTIP_LINES)
    {
        snprintf(lines[lineCount], sizeof(lines[lineCount]), "Speed: +%%%.f", cfg.speed * 100.0f);
        float w = Sol_MeasureText(lines[lineCount], UISCALE(bodyTextSize), SOL_FONT_ICE);
        if (w > maxWidth)
            maxWidth = w;
        lineCount++;
    }

    if (cfg.damage.amount > 0 && lineCount < MAX_TOOLTIP_LINES)
    {
        if (cfg.damage.amount > 0)
        {
            snprintf(lines[lineCount], sizeof(lines[lineCount]), "Damage: +%%%.f", cfg.damage.amount * 100.0f);
        }
        float w = Sol_MeasureText(lines[lineCount], UISCALE(bodyTextSize), SOL_FONT_ICE);
        if (w > maxWidth)
            maxWidth = w;
        lineCount++;
    }

    if (cfg.maxpower > 0 && lineCount < MAX_TOOLTIP_LINES)
    {
        if (cfg.maxpower > 0)
        {
            snprintf(lines[lineCount], sizeof(lines[lineCount]), "Max Power: +%%%.f", cfg.maxpower * 100.0f);
        }
        float w = Sol_MeasureText(lines[lineCount], UISCALE(bodyTextSize), SOL_FONT_ICE);
        if (w > maxWidth)
            maxWidth = w;
        lineCount++;
    }

    u8 totalBuffs = cfg.damage.buffMask;
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

    u32 totalEffects = cfg.damage.effectMask;
    if ((totalEffects & (EFFECTMASK_KNOCKBACK)) && lineCount < MAX_TOOLTIP_LINES)
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
    if ((totalEffects & EFFECTMASK_LIFESTEAL) && lineCount < MAX_TOOLTIP_LINES)
    {
        snprintf(lines[lineCount], sizeof(lines[lineCount]), "Lifesteal");
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
    vec2s pos    = {UISCALE(input->mouse.x), UISCALE(input->mouse.y - dims.y)};
    vec2s center = {pos.x + dims.x * 0.5f, pos.y + dims.y * 0.5f};

    // --- STAGE 3: DRAW BACKDROP GEOMETRY FIRST ---
    RectSSBO *bg = Sol_Render_GetNext_Rect(UILAYER_2);
    bg->pos      = (vec4s){pos.x, pos.y, 0, 1.0f};
    bg->color    = (vec4s){0.05f, 0.0f, 0.1f, alpha};
    bg->rect     = (vec4s){0, 0, dims.x, dims.y};

    RectSSBO *border  = Sol_Render_GetNext_Rect(UILAYER_2);
    border->pos       = (vec4s){pos.x, pos.y, 0, 1.0f};
    border->color     = (vec4s){0.0f, 0.0f, 0.02f, alpha};
    border->rect      = (vec4s){0, 0, dims.x, dims.y};
    border->uv        = (vec4s){0, 0, 1, 1};
    border->textureId = SOL_TEXTURE_CLOUD1;

    RectSSBO *border2  = Sol_Render_GetNext_Rect(UILAYER_2);
    border2->pos       = (vec4s){pos.x, pos.y, 0, 1.0f};
    border2->color     = (vec4s){0.0f, 0.0f, 0.2f, alpha};
    border2->rect      = (vec4s){0, 0, dims.x, dims.y};
    border2->uv        = (vec4s){0, 0, 1, 1};
    border2->textureId = SOL_TEXTURE_SWIRLFRAME;

    RectSSBO *border3  = Sol_Render_GetNext_Rect(UILAYER_2);
    border3->pos       = (vec4s){pos.x, pos.y, 0, 1.0f};
    border3->color     = (vec4s){1.0f, 1.0f, 1.0f, 0.15f};
    border3->rect      = (vec4s){0, 0, dims.x, dims.y};
    border3->uv        = (vec4s){0, 0, 1, 1};
    border3->textureId = SOL_TEXTURE_BORDER;

    // --- STAGE 4: RENDER THE TEXT ON TOP ---
    // Start layout downward from top padding limits
    float currentY = pos.y + paddingY + (UISCALE(headerSize) * 0.5f);

    SolFontDesc headerDesc = {
        .color = {1.0f, 0, 0, alpha / MAX_TOOLTIP_ALPHA},
        .size  = UISCALE(headerSize),
        .layer = UILAYER_3,
        .x     = center.x - headerWidth * 0.5f,
        .y     = currentY,
    };
    Sol_Render_DrawText2D(headerText, headerDesc);

    // Step below header bounds
    currentY += UISCALE(headerSize);

    // Flush out the remaining tracked strings
    for (int i = 0; i < lineCount; i++)
    {
        Render_Tooltip_Line(lines[i], center.x, currentY, bodyTextSize, alpha);
        currentY += ySpacing;
    }
}

typedef void (*TooltipDraw)(World *world, int id, UserInput *input, float alpha);
const TooltipDraw tooltip_funcs[TOOLTIPKIND_COUNT] = {
    [TOOLTIPKIND_CARD] = Item_Draw,
};

void Sol_Tooltip_Draw(World *world, int id, UserInput *input, float alpha)
{
    if (!world)
        return;
    forc(world, id, ScTooltip)
    {
        TooltipDraw draw = tooltip_funcs[c->kind];
        if (draw)
            draw(world, id, input, alpha);
    }
}