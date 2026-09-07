#include "world.h"
#include "sol_math.h"

void Camera_Tick(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_ScCamera *set = Sol_Comp_Set(world, ScCamera);
    for (int i = 0; i < set->cnt; i++)
    {
        int id           = set->dense[i];
        ScCamera *camera = &set->data[i];
        Xform xform = Xform_GetDraw(world, id);
        ScBody3 *body3   = Sol_Comp_Get(world, id, ScBody3);

        vec3s head = xform.pos;
        head.y += body3->dims.y * 0.5f;
        vec3s lookdir = (vec3s){0, 0, 1.0f};

        if (Sol_Comp_Has(world, id, ScCmd))
        {
            ScCmd *cmd = Sol_Comp_Get(world, id, ScCmd);
            lookdir    = cmd->lookdir;
        }

        vec3s invDir    = glms_vec3_scale(lookdir, -1.0f);
        vec3s offsetvec = glms_vec3_cross(lookdir, WORLD_UP);

        if (camera->desired_distance <= 0)
        {
            camera->desired_distance = 0;
            camera->current_distance = 0;
            camera->anchor           = head;
        }
        else
        {
            float factor = 1.0f - expf(-camera->lerpspeed * fdt);

            SolRayResult anchortrace = {0};
            bool offsetHit           = Sol_Raycast1(
                world, (SolRay){.start = head, .dir = offsetvec, .dist = camera->desired_offset, .mask = 1}, &anchortrace);

            float target_offset = camera->desired_offset;
            if (offsetHit)
            {
                target_offset = (-camera->desired_offset + (anchortrace.dist * 2.0f));
            }
            target_offset -= 0.2f;
            if (target_offset < camera->current_offset)
            {
                camera->current_offset = target_offset;
            }
            else
            {
                camera->current_offset = Sol_Math_Lerp(camera->current_offset, target_offset, factor);
            }

            camera->anchor = vecAdd(head, vecSca(offsetvec, camera->current_offset));

            SolRayResult dist_trace = {0};
            bool distanceHit        = Sol_Raycast1(
                world, (SolRay){.start = camera->anchor, .dir = invDir, .dist = camera->desired_distance, .mask = 1}, &dist_trace);

            float target_dist = camera->desired_distance;
            if (distanceHit)
            {
                target_dist = dist_trace.dist;
            }
            target_dist -= 0.2f;
            if (target_dist < camera->current_distance)
            {
                camera->current_distance = target_dist;
            }
            else
            {
                camera->current_distance = Sol_Math_Lerp(camera->current_distance, target_dist, factor);
            }
        }

        camera->pos = glms_vec3_add(camera->anchor, glms_vec3_scale(invDir, camera->current_distance));
        camera->dir = lookdir;

        // === Wallrun tilt ===
        float targetRoll = 0.0f;
        if (Sol_Comp_Has(world, id, ScMove3))
        {
            ScMove3 *move = Sol_Comp_Get(world, id, ScMove3);
            if (move->state == MOVE_WALLRUN)
            {
                vec3s dir   = vecSub(move->lastTouch, xform.pos);
                dir         = vecNorm(dir);
                vec3s right = vecCrs(camera->dir, WORLD_UP);
                float dot   = vecDot(right, dir);
                targetRoll  = -dot * 15.0f * (3.14159f / 180.0f);
            }
        }
        float rollFactor = 1.0f - expf(-camera->lerpspeed * fdt); // slower lerp for cinematic feel
        camera->roll     = Sol_Math_Lerp(camera->roll, targetRoll, rollFactor);
        camera->up       = glms_vec3_rotate(WORLD_UP, camera->roll, camera->dir); // rotate up around forward
    }
}

void Camera_Init(World *world)
{
}