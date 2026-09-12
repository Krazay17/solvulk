/*
 * File: c_ability.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-11
 * 
*/
#include "component.h"
#include "world.h"

const char *ability_state_name[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_IDLE] = "Idle",           [ABILITY_STATE_DASH] = "Dash",
    [ABILITY_STATE_FIREBALL] = "Fireball",   [ABILITY_STATE_PISTOL] = "Pistol",
    [ABILITY_STATE_SPINSLASH] = "Spinslash", [ABILITY_STATE_CLAW] = "Claw",
    [ABILITY_STATE_SHIELD] = "Shield",       [ABILITY_STATE_LASER] = "Laser",
    [ABILITY_STATE_WHIP] = "Whip",           [ABILITY_STATE_FIREBALLVOLLEY] = "FireballVolley",
};

const u32 ability_texture_map[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_IDLE]     = 0,
    [ABILITY_STATE_DASH]     = SOL_TEXTURE_DASH_CARD,
    [ABILITY_STATE_CLAW]     = SOL_TEXTURE_BLADE_CARD,
    [ABILITY_STATE_FIREBALL] = SOL_TEXTURE_FIREBALL_CARD,
    [ABILITY_STATE_PISTOL]   = SOL_TEXTURE_PISTOL_CARD,
    // [ABILITY_STATE_SPINSLASH] = ,
    // [ABILITY_STATE_SHIELD] = ,
    // [ABILITY_STATE_LASER] = ,
    // [ABILITY_STATE_WHIP] = ,
    // [ABILITY_STATE_FIREBALLVOLLEY] = ,
};


const AbilityConfig ability_base[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_IDLE] =
        {
            0,
        },
    [ABILITY_STATE_DASH] =
        {
            .duration   = 0.3f,
            .cooldown   = 1.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_FIREBALL] =
        {
            .duration        = 3.0f,
            .recoverDuration = 0.5f,
            .cooldown        = 0.0f,
            .damage          = 10.0f,
            .effectMask      = EFFECTMASK_KNOCKBACK,
            .buffMask        = BUFFKIND_FIRE,
            .maxpower        = 2.0f,
        },
    [ABILITY_STATE_PISTOL] =
        {
            .duration   = 0.5f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_SPINSLASH] =
        {
            .duration   = 0.5f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_CLAW] =
        {
            .duration   = 0.6f,
            .cooldown   = 0.1f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_SHIELD] =
        {
            .duration   = 0.5f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_LASER] =
        {
            .duration   = 0.5f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_WHIP] =
        {
            .duration   = 0.5f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_FIREBALLVOLLEY] =
        {
            .duration   = 0.5f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
};
