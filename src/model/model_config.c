#include "sol_core.h"

#include "model_i.h"

SolModelMasks model_masks[SOL_MODEL_COUNT];

const char *model_path[SOL_MODEL_COUNT] = {[SOL_MODEL_WIZARD] = "Wizard.glb",
                                           [SOL_MODEL_DUDE]   = "Dude.glb",
                                           [SOL_MODEL_BOX]    = "Box.glb",
                                           [SOL_MODEL_WORLD0] = "World0.glb",
                                           [SOL_MODEL_WORLD1] = "World1.glb",
                                           [SOL_MODEL_WORLD2] = "World2.glb"

};

/*
Wizard anims:
Anim: 0 idle
Anim: 1 fwd
Anim: 2 attack1

Dude anims:
Anim: 0 run
Anim: 1 idle
Anim: 2 fall
Anim: 3 jump
Anim: 4 frontFlip
Anim: 5 dash
Anim: 6 dashLeft
Anim: 7 dashBwd
Anim: 8 dashRight
Anim: 9 strafeRight
Anim: 10 strafeLeft
Anim: 11 fallLeft
Anim: 12 fallRight
Anim: 13 fallBwd
Anim: 14 attackRight
Anim: 15 attackLeft
Anim: 16 attackSpell
Anim: 17 runBwd
Anim: 18 blade
Anim: 19 bladeAir
Anim: 20 runStopLeft
Anim: 21 runStopFwd
Anim: 22 runStopRight
Anim: 23 knockback
Anim: 24 spinSlash
Anim: 25 LeftCharge
*/
const i32 model_anim_map[SOL_MODEL_COUNT][ANIM_COUNT] = {
    [SOL_MODEL_WIZARD] =
        {
            [ANIM_IDLE] = 0,       [ANIM_WALK_FWD] = 1,  [ANIM_WALK_BWD] = 1,   [ANIM_WALK_LEFT] = 1,
            [ANIM_WALK_RIGHT] = 1, [ANIM_JUMP] = 1,      [ANIM_FALL] = 1,       [ANIM_DASH_FWD] = 1,
            [ANIM_DASH_BWD] = 1,   [ANIM_DASH_LEFT] = 1, [ANIM_DASH_RIGHT] = 1, [ANIM_ABILITY0] = 2,
            [ANIM_ABILITY1] = 2,   [ANIM_ABILITY2] = 2,  [ANIM_ABILITY3] = 2,   [ANIM_ABILITY4] = 2,
            [ANIM_ABILITY5] = 2,   [ANIM_ABILITY6] = 2,  [ANIM_ABILITY7] = 2,   [ANIM_ABILITY8] = 2,
            [ANIM_ABILITY9] = 2,
        },
    [SOL_MODEL_DUDE] =
        {
            [ANIM_IDLE] = 1,         [ANIM_WALK_FWD] = 0,      [ANIM_WALK_BWD] = 17,  [ANIM_WALK_LEFT] = 10,
            [ANIM_WALK_RIGHT] = 9,   [ANIM_JUMP] = 3,          [ANIM_FALL] = 2,       [ANIM_DASH_FWD] = 5,
            [ANIM_DASH_BWD] = 7,     [ANIM_DASH_LEFT] = 6,     [ANIM_DASH_RIGHT] = 8, [ANIM_CHARGE_LEFT] = 25,
            [ANIM_ATTACK_LEFT] = 15, [ANIM_ATTACK_RIGHT] = 14, [ANIM_ABILITY0] = 14,  [ANIM_ABILITY1] = 14,
            [ANIM_ABILITY2] = 14,    [ANIM_ABILITY3] = 14,     [ANIM_ABILITY4] = 25,  [ANIM_ABILITY5] = 14,
            [ANIM_ABILITY6] = 14,    [ANIM_ABILITY7] = 14,     [ANIM_ABILITY8] = 14,  [ANIM_ABILITY9] = 14,
        },
};
