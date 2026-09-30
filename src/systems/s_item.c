#include "world.h"

enum
{
    COOLDOWN,
    DAMAGE,
    MAXPOWER,
    SPEED,
    PARAMCOUNT,
};

float item_rarity_scale[PARAMCOUNT] = {
    [COOLDOWN] = 0.1f,
    [DAMAGE]   = 0.1f,
    [MAXPOWER] = 0.1f,
    [SPEED]    = 0.1f,
};

AbilityConfig Sol_Item_GetMods(SolItem item)
{
    AbilityConfig conf = {0};
    if (item.abilityKind > 0 && item.abilityKind < ABILITYKIND_COUNT)
    {
        conf.cooldown   = (item_rarity_scale[COOLDOWN] * (float)item.rarity);
        conf.damage     = (item_rarity_scale[DAMAGE] * (float)item.rarity);
        conf.maxpower   = (item_rarity_scale[MAXPOWER] * (float)item.rarity);
        conf.speed      = (item_rarity_scale[SPEED] * (float)item.rarity);
        conf.buffMask   = item.buffMask;
        conf.effectMask = item.effectMask;
    }
    return conf;
}

AbilityConfig Sol_Item_ApplyMods(AbilityConfig conf, SolItem item)
{
    if (item.abilityKind > 0 && item.abilityKind < ABILITYKIND_COUNT)
    {
        conf.cooldown *= 1.0f - (item_rarity_scale[COOLDOWN] * (float)item.rarity);
        conf.damage *= 1.0f + (item_rarity_scale[DAMAGE] * (float)item.rarity);
        conf.maxpower *= 1.0f + (item_rarity_scale[MAXPOWER] * (float)item.rarity);
        conf.speed *= 1.0f + (item_rarity_scale[SPEED] * (float)item.rarity);
        conf.buffMask |= item.buffMask;
        conf.effectMask |= item.effectMask;
    }
    return conf;
}