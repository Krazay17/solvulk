#include "sol_core.h"

#include "model_i.h"

const CompModel model_kinds[SOL_MODEL_COUNT] = {
    [MODELKIND_DUDE] =
        {
            .yawOffset = GLM_PI_2f,
        },
};

SolModelMasks model_masks[SOL_MODEL_COUNT];

const char *model_path[SOL_MODEL_COUNT] = {
    [MODELKIND_WIZARD] = "Wizard.glb",
    [MODELKIND_DUDE]   = "Dude.glb",
    [SOL_MODEL_BOX] = "Box.glb",
    [SOL_MODEL_WORLD0] = "World0.glb",
    [SOL_MODEL_WORLD1] = "World1.glb",
    [SOL_MODEL_WORLD2] = "World2.glb",
};

/*
Wizard anims:
Anim: 0 idle
Anim: 1 fwd
Anim: 2 attack1

Dude anims:
Anim: 0 idle
Anim: 1 WalkFwd
Anim: 2 WalkLeft
Anim: 3 WalkBwd
Anim: 4 WalkRight
Anim: 5 fall
Anim: 6 jump
Anim: 7 frontFlip
Anim: 8 dash
Anim: 9 dashLeft
Anim: 10 dashBwd
Anim: 11 dashRight
Anim: 12 fallLeft
Anim: 13 fallBwd
Anim: 14 fallRight
Anim: 15 attackRight
Anim: 16 attackLeft
Anim: 17 attackSpell
Anim: 18 blade
Anim: 19 bladeAir
Anim: 20 runStopLeft
Anim: 21 runStopFwd
Anim: 22 runStopRight
Anim: 23 knockback
Anim: 24 spinSlash
Anim: 25 LeftCharge
Anim: 26 CrouchWalkFwd
Anim: 27 Slide
Anim: 28 Stunned
Anim: 29 LeftChannel
Anim: 30 RightChannel
*/
const i32 model_anim_map[SOL_MODEL_COUNT][ANIM_COUNT] = {
    [MODELKIND_WIZARD] =
        {
            [ANIM_IDLE] = 0,       [ANIM_WALK_FWD] = 1,  [ANIM_WALK_BWD] = 1,   [ANIM_WALK_LEFT] = 1,
            [ANIM_WALK_RIGHT] = 1, [ANIM_JUMP] = 1,      [ANIM_FALL] = 1,       [ANIM_DASH_FWD] = 1,
            [ANIM_DASH_BWD] = 1,   [ANIM_DASH_LEFT] = 1, [ANIM_DASH_RIGHT] = 1, [ANIM_ABILITY0] = 2,
            [ANIM_ABILITY1] = 2,   [ANIM_ABILITY2] = 2,  [ANIM_ABILITY3] = 2,   [ANIM_ABILITY4] = 2,
            [ANIM_ABILITY5] = 2,   [ANIM_ABILITY6] = 2,  [ANIM_ABILITY7] = 2,   [ANIM_ABILITY8] = 2,
            [ANIM_ABILITY9] = 2,   [ANIM_DEATH] = 3,     [ANIM_STUN] = 4,
        },
    [MODELKIND_DUDE] =
        {
            [ANIM_IDLE]             = 0,
            [ANIM_WALK_FWD]         = 1,
            [ANIM_WALK_LEFT]        = 2,
            [ANIM_WALK_BWD]         = 3,
            [ANIM_WALK_RIGHT]       = 4,
            [ANIM_JUMP]             = 6,
            [ANIM_FALL]             = 5,
            [ANIM_DASH_FWD]         = 8,
            [ANIM_DASH_LEFT]        = 9,
            [ANIM_DASH_BWD]         = 10,
            [ANIM_DASH_RIGHT]       = 11,
            [ANIM_CHARGE_LEFT]      = 25,
            [ANIM_CHANNEL_LEFT]     = 29,
            [ANIM_CHANNEL_RIGHT]    = 30,
            [ANIM_ATTACK_LEFT]      = 16,
            [ANIM_ATTACK_RIGHT]     = 15,
            [ANIM_SPINSLASH]        = 24,
            [ANIM_ABILITY0]         = 24,
            [ANIM_ABILITY1]         = 15,
            [ANIM_ABILITY2]         = 15,
            [ANIM_ABILITY3]         = 15,
            [ANIM_ABILITY4]         = 25,
            [ANIM_ABILITY5]         = 15,
            [ANIM_ABILITY6]         = 15,
            [ANIM_ABILITY7]         = 15,
            [ANIM_ABILITY8]         = 15,
            [ANIM_ABILITY9]         = 15,
            [ANIM_CROUCHWALK_FWD]   = 26,
            [ANIM_CROUCHWALK_BWD]   = 26,
            [ANIM_CROUCHWALK_LEFT]  = 26,
            [ANIM_CROUCHWALK_RIGHT] = 26,
            [ANIM_SLIDE_FWD]        = 27,
            [ANIM_SLIDE_BWD]        = 27,
            [ANIM_SLIDE_LEFT]       = 27,
            [ANIM_SLIDE_RIGHT]      = 27,
            [ANIM_DEATH]            = 23,
            [ANIM_STUN]             = 28,
        },
};
