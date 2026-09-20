#include "world.h"
#include "sol_core.h"
#include "sol_math.h"
#include "sol_user.h"

struct Aim
{
    vec3s pos, dir, norm;
    bool hit;
    int target;
};
static struct Aim Sol_Player_SetParallaxAim(World *world, int id, vec3s headpos, vec3s lookpos, vec3s lookdir,
                                            float range, float hitdepth)
{
    struct Aim aim = {.pos = Sol_AddScaledDir(lookpos, lookdir, range)};
    aim.dir        = glms_vec3_normalize(glms_vec3_sub(aim.pos, headpos));

    SolRay ray = {
        .start     = lookpos,
        .mask      = COLLAYER_ALL,
        .dir       = lookdir,
        .dist      = range,
        .ignoreEnt = id,
        .debug     = true,
    };
    SolRayResult aimTrace = {0};
    aim.hit               = Sol_Raycast1(world, ray, &aimTrace);

    hitdepth        = Sol_Math_Lerp(hitdepth, 0.0f, aimTrace.t / range);
    vec3s add_depth = vecSca(lookdir, hitdepth);
    aim.pos         = vecAdd(Sol_AddScaledDir(lookpos, lookdir, aimTrace.t), add_depth);
    aim.dir         = vecNorm(vecSub(aim.pos, headpos));

    if (solState.debug)
    {
        SolSphere *debug_sphere = Sol_Debug_NewSphere(world, 0.5f);
        debug_sphere->color     = VEC4_GREEN;
        debug_sphere->pos       = aim.pos;
        debug_sphere->radius    = 0.2f;
    }

    aim.target = aimTrace.entId > -1 ? aimTrace.entId : -1;
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
        cmd->action_state_prev = cmd->actionState;
        cmd->yaw               = sol_user.yaw;
        cmd->pitch             = sol_user.pitch;
        cmd->isStrafing        = sol_user.mouse_locked;
        cmd->actionState       = sol_user.actions;

        cmd->lookdir  = vecNorm(Sol_Vec3_FromYawPitch(sol_user.yaw, sol_user.pitch));
        cmd->aimdir   = cmd->lookdir;
        cmd->wishdir  = CalcWishdir3(sol_user.actions, cmd->lookdir, WORLD_UP, false);
        cmd->wishdir2 = CalcWishDir2(sol_user.actions);

        if (Sol_Comp_Has(world, id, ScCamera))
        {
            ScCamera *camera = Sol_Comp_Get(world, id, ScCamera);
            struct Aim aim   = Sol_Player_SetParallaxAim(world, id, Sol_Body3_GetHead(world, id), camera->anchor,
                                                         camera->dir, 50.0f, 2.5f);
            cmd->aimpos      = aim.pos;
            cmd->aimdir      = aim.dir;
            cmd->target      = aim.target;
        }
    }
}

void Player_Init(World *world)
{
}

void Player_Deinit(World *world)
{
}
