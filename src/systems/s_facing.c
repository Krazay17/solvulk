#include "world.h"
#include "sol_math.h"

void Facing_Tick(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_ScCmd *set = Sol_Comp_Set(world, ScCmd);
    for (int i = 0; i < set->cnt; i++)
    {
        int id     = set->dense[i];
        ScCmd *cmd = &set->data[i];

        // MODE 1: Strafing / Aiming — Snap or smooth towards camera yaw
        if (cmd->isStrafing)
        {
            world->xform.rot[id] = Sol_Quat_FromYawPitch(cmd->yaw, 0.0f);
        }
        // MODE 2: Free Movement — Smoothly turn model toward movement wishdir
        else if (glms_vec3_norm(cmd->wishdir) > 0.001f)
        {
            float target_yaw    = atan2f(cmd->wishdir.x, cmd->wishdir.z);
            versors target_quat = Sol_Quat_FromYawPitch(target_yaw, 0.0f);

            float turn_speed = 10.0f;
            float factor     = 1.0f - expf(-turn_speed * fdt);

            world->xform.rot[id] = glms_quat_slerp(world->xform.rot[id], target_quat, factor);
        }
    }
}