#include "ability_i.h"

#define SCRIPT_STATE_FUNCS                                                                                             \
    {Script_State_Update, Script_State_Enter, Script_State_Exit, Script_State_CanExit, Script_State_CanEnter}

const StateFunc ability_state_func[] = {
    [ABILITY_STATE_IDLE] =
        {
            IdleAbility_State_Update,
            IdleAbility_State_Enter,
            IdleAbility_State_Exit,
            IdleAbility_State_CanExit,
            IdleAbility_State_CanEnter,
        },
    [ABILITY_STATE_DASH] =
        {
            ADash_State_Update,
            ADash_State_Enter,
            ADash_State_Exit,
            ADash_State_CanExit,
            ADash_State_CanEnter,
        },
    [ABILITY_STATE_FIREBALL] =
        {
            Fireball_State_Update,
            Fireball_State_Enter,
            Fireball_State_Exit,
            Fireball_State_CanExit,
            Fireball_State_CanEnter,
            Fireball_State_Draw,
        },
    [ABILITY_STATE_SHIELD] =
        {
            Shield_State_Update,
            Shield_State_Enter,
            Shield_State_Exit,
            Shield_State_CanExit,
            Shield_State_CanEnter,
            Shield_State_Draw,
        },
    [ABILITY_STATE_CLAW] =
        {
            Claw_State_Update,
            Claw_State_Enter,
            Claw_State_Exit,
            Claw_State_CanExit,
            Claw_State_CanEnter,
        },
    [ABILITY_STATE_PISTOL] =
        {
            Pistol_State_Update,
            Pistol_State_Enter,
            Pistol_State_Exit,
            Pistol_State_CanExit,
            Pistol_State_CanEnter,
        },
    [ABILITY_STATE_SPINSLASH] =
        {
            Spinslash_State_Update,
            Spinslash_State_Enter,
            Spinslash_State_Exit,
            Spinslash_State_CanExit,
            Spinslash_State_CanEnter,
        },
    [ABILITY_STATE_LASER] =
        {
            Laser_State_Update,
            Laser_State_Enter,
            Laser_State_Exit,
            Laser_State_CanExit,
            Laser_State_CanEnter,
            Laser_State_Draw,
        },
    [ABILITY_STATE_WHIP] =
        {
            Whip_State_Update,
            Whip_State_Enter,
            Whip_State_Exit,
            Whip_State_CanExit,
            Whip_State_CanEnter,
        },
    [ABILITY_STATE_FIREBALLVOLLEY] = SCRIPT_STATE_FUNCS,
};
