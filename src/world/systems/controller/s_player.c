#include "world.h"
#include "sol_math.h"
#include "sol_user.h"

struct Aim
{
    vec3s dir;
    vec3s pos;
    int target;
};
static struct Aim Sol_Player_SetParallaxAim(World *world, int id, vec3s headpos, vec3s lookpos, vec3s lookdir,
                                            float range, float hitdepth)
{
    struct Aim aim        = {.dir = lookdir};
    SolRayResult aimTrace = {.pos = vecAdd(lookpos, vecSca(lookdir, range))};
    bool hit              = Sol_Raycast1(world,
                                         (SolRay){
                                             .start = lookpos,
                                             .mask  = COLLAYER_ALL,
                                             .dir   = lookdir,
                                             .dist  = range,
                                         },
                                         &aimTrace);
    // Add slight depth into hit
    aimTrace.pos       = vecAdd(aimTrace.pos, vecSca(lookdir, hitdepth));
    aim.pos            = aimTrace.pos;
    vec3s dirFromTrace = glms_vec3_normalize(glms_vec3_sub(aimTrace.pos, headpos));
    aim.dir            = vecDot(dirFromTrace, lookdir) > 0.7f ? dirFromTrace : lookdir;
    aim.target         = aimTrace.entId > -1 ? aimTrace.entId : -1;
    return aim;
}

void Player_Tick(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_ScPlayer *set = Sol_Comp_Set(world, ScPlayer);
    for (int i = 0; i < set->cnt; i++)
    {
        int id           = set->dense[i];
        ScPlayer *player = &set->data[i];
        ScCmd *cmd       = Sol_Comp_Get(world, id, ScCmd);
        if (!cmd)
            continue;
        cmd->yaw         = sol_user.yaw;
        cmd->pitch       = sol_user.pitch;
        cmd->isStrafing  = sol_user.mouse_locked;
        cmd->actionState = sol_user.actions;

        cmd->lookdir  = vecNorm(Sol_Vec3_FromYawPitch(sol_user.yaw, sol_user.pitch));
        cmd->wishdir  = CalcWishdir3(sol_user.actions, cmd->lookdir, WORLD_UP, false);
        cmd->wishdir2 = CalcWishDir2(sol_user.actions);

        if (Sol_Comp_Has(world, id, ScBody3))
        {
            ScBody3 *body = Sol_Comp_Get(world, id, ScBody3);
            vec3s head    = world->xform.pos[id];
            head.y += body->dims.y * 0.4f;
            cmd->headpos = head;
        }

        if (Sol_Comp_Has(world, id, ScCamera))
        {
            ScCamera *camera = Sol_Comp_Get(world, id, ScCamera);
            struct Aim aim = Sol_Player_SetParallaxAim(world, id, cmd->headpos, camera->pos, camera->dir, 50.0f, 0.5f);
            cmd->aimpos    = aim.pos;
            cmd->aimdir    = aim.dir;
            cmd->target    = aim.target;
        }

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
