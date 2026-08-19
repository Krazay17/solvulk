#include "model.h"
#include "model/s_model.h"

const CompModel model_kinds[SOL_MODEL_COUNT] = {
    [MODELKIND_DUDE] =
        {
            // .yawOffset = GLM_PI_2f,
            .yOffset = -0.825f,
        },
    [MODELKIND_WIZARD] =
        {
            .yOffset = -1.5f,
        },
    [MODELKIND_ZORGON] =
        {
            .yOffset = -0.8f,
        },
};

const char *model_path[SOL_MODEL_COUNT] = {
    [MODELKIND_WIZARD]      = "Wizard.glb",
    [MODELKIND_DUDE]        = "Dude.glb",
    [MODELKIND_ZORGON]      = "Zorgon.glb",
    [MODELKIND_WEAPONBLADE] = "WeaponBlade.glb",
    [SOL_MODEL_BOX]         = "Box.glb",
    [SOL_MODEL_WORLD0]      = "World0.glb",
    [MODELKIND_WALL]        = "Wall.glb",
    [SOL_MODEL_WORLD1]      = "World1.glb",
    [SOL_MODEL_WORLD2]      = "World2.glb",
    [SOL_MODEL_WORLD6]      = "World6.glb",
    [SOL_MODEL_WORLD7]      = "World7.glb",
    [SOL_MODEL_WORLD8]      = "World8.glb",
    [SOL_MODEL_WORLD9]      = "World9.glb",
    [SOL_MODEL_WORLD10]     = "World10.glb",
    [MODELKIND_FLOOR]       = "BlackRockFloor.glb",
};

/*
Wizard anims:
Anim: 0 idle
Anim: 1 fwd
Anim: 2 attack1

Dude anims:
Model: Scene, Anim: 0 idle
Model: Scene, Anim: 1 WalkFwd
Model: Scene, Anim: 2 WalkLeft
Model: Scene, Anim: 3 WalkBwd
Model: Scene, Anim: 4 WalkRight
Model: Scene, Anim: 5 fall
Model: Scene, Anim: 6 jump
Model: Scene, Anim: 7 frontFlip
Model: Scene, Anim: 8 dash
Model: Scene, Anim: 9 dashLeft
Model: Scene, Anim: 10 dashBwd
Model: Scene, Anim: 11 dashRight
Model: Scene, Anim: 12 fallLeft
Model: Scene, Anim: 13 fallBwd
Model: Scene, Anim: 14 fallRight
Model: Scene, Anim: 15 attackRight
Model: Scene, Anim: 16 attackLeft
Model: Scene, Anim: 17 attackSpell
Model: Scene, Anim: 18 blade
Model: Scene, Anim: 19 bladeAir
Model: Scene, Anim: 20 runStopLeft
Model: Scene, Anim: 21 runStopFwd
Model: Scene, Anim: 22 runStopRight
Model: Scene, Anim: 23 knockback
Model: Scene, Anim: 24 spinSlash
Model: Scene, Anim: 25 LeftCharge
Model: Scene, Anim: 26 CrouchWalkFwd
Model: Scene, Anim: 27 SlideFwd
Model: Scene, Anim: 28 Stunned
Model: Scene, Anim: 29 LeftChannel
Model: Scene, Anim: 30 RightChannel
Model: Scene, Anim: 31 WallJumpLeft
Model: Scene, Anim: 32 WallJumpRight
Model: Scene, Anim: 33 Mantle
Model: Scene, Anim: 34 WallClimb
Model: Scene, Anim: 35 WallrunRight
Model: Scene, Anim: 36 WallrunLeft
Model: Scene, Anim: 37 MantleRoll
Model: Scene, Anim: 38 MantleRollRev
Model: Scene, Anim: 39 Jump2
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
            [ANIM_JUMP]             = 39,
            [ANIM_FLIPJUMP]         = 7,
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
            [ANIM_WALLJUMP_LEFT]    = 31,
            [ANIM_WALLJUMP_RIGHT]   = 32,
            [ANIM_WALLRUN_FWD]      = 34,
            [ANIM_MANTLE]           = 33,
            [ANIM_MANTLE_ROLL]      = 37,
            [ANIM_WALLRUN_LEFT]     = 35,
            [ANIM_WALLRUN_RIGHT]    = 36,
            [ANIM_BACKFLIP]         = 38,
        },
    [MODELKIND_ZORGON] =
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
