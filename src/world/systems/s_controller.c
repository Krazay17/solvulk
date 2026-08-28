#include "world.h"
#include "sol_math.h"

void Controller_Tick(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_SolController *set = Sol_Comp_Set(world, SolController);
    for (int i = 0; i < set->cnt; i++)
    {
        int            id    = set->dense[i];
        SolController *cont  = &set->data[i];
        SolXform      *xform = Sol_Comp_Get(world, id, SolXform);

        vec3s lookdir   = vecNorm(Sol_Vec3_FromYawPitch(cont->yaw, cont->pitch));
        cont->lookdir   = lookdir;
        cont->wishdir   = CalcWishdir3(cont->actionState, cont->lookdir, WORLD_UP, false);
        cont->wishdirY  = CalcWishdir3(cont->actionState, cont->lookdir, WORLD_UP, true);
        cont->wishdir2d = CalcWishDir2(cont->actionState);
        if (Sol_Comp_Has(world, id, SolAbility))
        {
            SolAbility *ability = Sol_Comp_Get(world, id, SolAbility);
            // for (int i = 0; i < ABILITY_SLOTS; i++)
            // {
            //     int  ability_mask          = BITC(ACTION_ABILITY1 + i);
            //     bool isDown                = cont->actionState & ability_mask;
            //     ability->stateData[i].held = isDown;

            //     if (isDown)
            //     {
            //         Sol_Ability_SetState(world, id, ability->action_map[i], i, false);
            //     }
            // }
        }
        if (Sol_Comp_Has(world, id, SolBody3))
        {
            SolBody3 *body3  = Sol_Comp_Get(world, id, SolBody3);
            vec3s     aimpos = xform->pos;
            aimpos.y += body3->dims.y;
            cont->aimpos = aimpos;

            if (cont->isStrafing)
                xform->rot = Sol_Quat_FromYawPitch(cont->yaw, 0);
            else if (glms_vec3_norm(cont->wishdir) > 0.001f)
            {
                float   target_entity_yaw = atan2f(cont->wishdir.x, cont->wishdir.z);
                versors target_quat       = Sol_Quat_FromYawPitch(target_entity_yaw, 0);

                // Smoothly turn the model toward the movement direction
                float turn_speed = 10.0f; // Higher numbers = faster turns
                float factor     = 1.0f - expf(-turn_speed * fdt);

                xform->rot = glms_quat_slerp(xform->rot, target_quat, factor);
            }
        }
        else if (Sol_Comp_Has(world, id, SolBody2))
        {
            SolBody2 *body2 = Sol_Comp_Get(world, id, SolBody2);

            if (glms_vec2_norm(cont->wishdir2d) > 0.001f)
            {
                float   target_entity_yaw = atan2f(cont->wishdir2d.x, 0);
                versors target_quat       = Sol_Quat_FromYawPitch(target_entity_yaw, 0);

                // Smoothly turn the model toward the movement direction
                float turn_speed = 10.0f; // Higher numbers = faster turns
                float factor     = 1.0f - expf(-turn_speed * fdt);

                xform->rot = glms_quat_slerp(xform->rot, target_quat, factor);
            }
        }
    }
}

void Controller_Init(World *world)
{
}

void Controller_Deinit(World *world)
{
}

void Sol_Controller_SetParallaxAim(World *world, int id, vec3s lookpos, vec3s lookdir, float range, float hitdepth)
{
    SolController *cont = Sol_Comp_Get(world, id, SolController);

    SolRayResult aimTrace;
    int          hits = Sol_Body3_Raycast(world,
                                          (SolRay){
                                              .pos       = lookpos,
                                              .ignoreEnt = id,
                                              .mask      = COLLISIONGROUP_PAWN | COLLISIONGROUP_WORLD,
                                              .dir       = lookdir,
                                              .dist      = range,
                                          },
                                          &aimTrace, 1);
    if (hits < 1)
        return;
    aimTrace.pos       = vecAdd(aimTrace.pos, vecSca(lookdir, hitdepth));
    vec3s dirFromTrace = glms_vec3_normalize(glms_vec3_sub(aimTrace.pos, cont->aimpos));
    cont->aimdir       = vecDot(dirFromTrace, lookdir) > 0.7f ? dirFromTrace : lookdir;
    cont->aimHitEnt    = aimTrace.entId > -1 ? aimTrace.entId : -1;
}