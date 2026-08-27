#include "interact/si_interact.h"
#include "sol_core.h"
#include "world.h"
#include "sol_math.h"
#include "input.h"
#include "font.h"
#include "render/render.h"

#include "item/s_item.h"
#include "ability/s_ability.h"

const TooltipDraw tooltip_funcs[TOOLTIPKIND_COUNT] = {
    [TOOLTIPKIND_CARD] = Tooltip_Card_Draw,
};

void Sol_Tooltip_Draw(SolUserHit user_hit, float alpha, double dt, double time)
{
    World *world = user_hit.hoverWorld;
    int    id    = user_hit.hoverId;
    if (alpha <= 0.0f || !world || id < 1 || user_hit.hoverId == user_hit.focusId)
        return;
    CompTooltip *tooltip = &world->tooltips[id];
    tooltip_funcs[tooltip->kind](world, id, alpha, dt, time);
}

static void Render_Tooltip_Line(const char *str, float centerX, float y, float size, float alpha)
{
    float       width = Sol_MeasureText(str, UISCALE(size), SOL_FONT_ICE);
    SolFontDesc desc  = {
        .color = {0.0f, 1.0f, 0.0f, alpha / MAX_TOOLTIP_ALPHA},
        .size  = UISCALE(size),
        .str   = str,
        .x     = centerX - width * 0.5f,
        .y     = y,
    };
    Sol_Render_DrawText2D(desc);
}

void Tooltip_Card_Draw(World *world, int id, float alpha, double dt, double time)
{
    if (!(world->masks[id] & BITC(HAS_TOOLTIP)))
        return;

    CompTooltip *tooltip = &world->tooltips[id];
    if (!WHasSys(world, WORLD_SYS_ITEM))
        return;
    SolItem *item = {0}; // &world->items[id].item;
    return;
    AbilityConfig cfg = {0};
    // if (item)
    //     cfg = item->ability;

    // --- STAGE 1: COMPUTE STRINGS & MEASURE WIDTHS ---
    // Keep a local stack array of the lines we need to draw
    char lines[MAX_TOOLTIP_LINES][64];
    int  lineCount = 0;

    // Track the raw width maximums
    float maxWidth = 0.0f;

    // 1. Measure and buffer Header
    const char *headerText  = ability_names[item->ability.state];
    float       headerSize  = 15.0f;
    float       headerWidth = Sol_MeasureText(headerText, UISCALE(headerSize), SOL_FONT_ICE);
    if (headerWidth > maxWidth)
        maxWidth = headerWidth;

    // 2. Buffer & Measure body lines
    float bodyTextSize = 10.0f;

    if (cfg.cooldown > 0.0f && lineCount < MAX_TOOLTIP_LINES)
    {
        snprintf(lines[lineCount], sizeof(lines[lineCount]), "Cooldown: %.1fs", cfg.cooldown);
        float w = Sol_MeasureText(lines[lineCount], UISCALE(bodyTextSize), SOL_FONT_ICE);
        if (w > maxWidth)
            maxWidth = w;
        lineCount++;
    }
    if (cfg.duration > 0.0f && lineCount < MAX_TOOLTIP_LINES)
    {
        snprintf(lines[lineCount], sizeof(lines[lineCount]), "Duration: %.1fs", cfg.duration);
        float w = Sol_MeasureText(lines[lineCount], UISCALE(bodyTextSize), SOL_FONT_ICE);
        if (w > maxWidth)
            maxWidth = w;
        lineCount++;
    }

    float totalDamage = cfg.damage + ability_base[item->ability.state].damage;
    if (totalDamage > 0 && lineCount < MAX_TOOLTIP_LINES)
    {
        if (cfg.damage > 0)
        {
            snprintf(lines[lineCount], sizeof(lines[lineCount]), "Damage: %.0f (+%.0f)", totalDamage, cfg.damage);
        }
        else
        {
            snprintf(lines[lineCount], sizeof(lines[lineCount]), "Damage: %.0f", totalDamage);
        }
        float w = Sol_MeasureText(lines[lineCount], UISCALE(bodyTextSize), SOL_FONT_ICE);
        if (w > maxWidth)
            maxWidth = w;
        lineCount++;
    }

    u8 totalBuffs = cfg.buffMask | ability_base[item->ability.state].buffMask;
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

    u32 totalEffects = cfg.effectMask | ability_base[item->ability.state].effectMask;
    if ((totalEffects & (EFFECTMASK_KNOCKBACK | EFFECTMASK_KNOCKBACK_STRONG)) && lineCount < MAX_TOOLTIP_LINES)
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
    if ((totalEffects & EFFECTMASK_HEALONHIT) && lineCount < MAX_TOOLTIP_LINES)
    {
        snprintf(lines[lineCount], sizeof(lines[lineCount]), "Heal on Hit");
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
    vec2s pos    = {(float)Sol_Input_GetMouse().x, (float)Sol_Input_GetMouse().y - dims.y};
    vec2s center = {pos.x + dims.x * 0.5f, pos.y + dims.y * 0.5f};

    // --- STAGE 3: DRAW BACKDROP GEOMETRY FIRST ---
    RectSSBO *bg = Sol_Render_GetNext_Rect();
    bg->color    = (vec4s){0.05f, 0.0f, 0.1f, alpha};
    bg->rect     = (vec4s){pos.x, pos.y, dims.x, dims.y};
    bg->scale    = 1.0f;
    bg->fill     = 1.0f;

    RectSSBO *border  = Sol_Render_GetNext_Rect();
    border->color     = (vec4s){0.0f, 0.0f, 0.02f, alpha};
    border->rect      = (vec4s){pos.x, pos.y, dims.x, dims.y};
    border->scale     = 1.0f;
    border->fill      = 1.0f;
    border->uv        = (vec4s){0, 0, 1, 1};
    border->textureID = SOL_TEXTURE_CLOUD1;

    RectSSBO *border2  = Sol_Render_GetNext_Rect();
    border2->color     = (vec4s){0.0f, 0.0f, 0.2f, alpha};
    border2->rect      = (vec4s){pos.x, pos.y, dims.x, dims.y};
    border2->scale     = 1.0f;
    border2->fill      = 1.0f;
    border2->uv        = (vec4s){0, 0, 1, 1};
    border2->textureID = SOL_TEXTURE_SWIRLFRAME;

    RectSSBO *border3  = Sol_Render_GetNext_Rect();
    border3->color     = (vec4s){1.0f, 1.0f, 1.0f, 0.15f};
    border3->rect      = (vec4s){pos.x, pos.y, dims.x, dims.y};
    border3->scale     = 1.0f;
    border3->fill      = 1.0f;
    border3->uv        = (vec4s){0, 0, 1, 1};
    border3->textureID = SOL_TEXTURE_BORDER;

    // --- STAGE 4: RENDER THE TEXT ON TOP ---
    // Start layout downward from top padding limits
    float currentY = pos.y + paddingY + (UISCALE(headerSize) * 0.5f);

    SolFontDesc headerDesc = {
        .color = {1.0f, 0, 0, alpha / MAX_TOOLTIP_ALPHA},
        .size  = UISCALE(headerSize),
        .str   = headerText,
        .x     = center.x - headerWidth * 0.5f,
        .y     = currentY,
    };
    Sol_Render_DrawText2D(headerDesc);

    // Step below header bounds
    currentY += UISCALE(headerSize);

    // Flush out the remaining tracked strings
    for (int i = 0; i < lineCount; i++)
    {
        Render_Tooltip_Line(lines[i], center.x, currentY, bodyTextSize, alpha);
        currentY += ySpacing;
    }
}
