#include "world.h"
#include "sol_math.h"
#include "sol_user.h"

void Player_Tick(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_ScPlayer *set = Sol_Comp_Set(world, ScPlayer);
    for (int i = 0; i < set->cnt; i++)
    {
        int       id     = set->dense[i];
        ScPlayer *player = &set->data[i];
        ScCmd    *cmd    = Sol_Comp_Get(world, id, ScCmd);
        if (!cmd)
            continue;
        cmd->yaw         = sol_user.yaw;
        cmd->pitch       = sol_user.pitch;
        cmd->isStrafing  = sol_user.mouse_locked;
        cmd->actionState = sol_user.actions;
        cmd->lookdir     = vecNorm(Sol_Vec3_FromYawPitch(sol_user.yaw, sol_user.pitch));
        cmd->wishdir     = CalcWishdir3(sol_user.actions, cmd->lookdir, WORLD_UP, false);
        cmd->wishdir2    = CalcWishDir2(sol_user.actions);

        if (Sol_Comp_Has(world, id, ScAbility))
        {
            ScAbility *ability = Sol_Comp_Get(world, id, ScAbility);
            // for (int i = 0; i < ABILITY_SLOTS; i++)
            // {
            //     int  ability_mask          = BITC(ACTION_ABILITY1 + i);
            //     bool isDown                = cmd->actionState & ability_mask;
            //     ability->stateData[i].held = isDown;

            //     if (isDown)
            //     {
            //         Sol_Ability_SetState(world, id, ability->action_map[i], i, false);
            //     }
            // }
        }
    }
}

void Player_Init(World *world)
{
}

void Player_Deinit(World *world)
{
}

void Sol_Player_SetParallaxAim(World *world, int id, vec3s lookpos, vec3s lookdir, float range, float hitdepth)
{
    ScPlayer *player = Sol_Comp_Get(world, id, ScPlayer);
    ScCmd    *cmd    = Sol_Comp_Get(world, id, ScCmd);

    SolRayResult aimTrace;
    int          hit = Sol_Raycast1D(world,
                                     (SolRay){
                                         .start = lookpos,
                                         .mask  = COLLISIONGROUP_PAWN | COLLISIONGROUP_WORLD,
                                         .dir   = lookdir,
                                         .dist  = range,
                                     },
                                     &aimTrace, 1);
    if (!hit)
        return;
    aimTrace.pos       = vecAdd(aimTrace.pos, vecSca(lookdir, hitdepth));
    vec3s dirFromTrace = glms_vec3_normalize(glms_vec3_sub(aimTrace.pos, cmd->aimpos));
    cmd->aimdir        = vecDot(dirFromTrace, lookdir) > 0.7f ? dirFromTrace : lookdir;
    cmd->target        = aimTrace.entId > -1 ? aimTrace.entId : -1;
}