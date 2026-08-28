#include "world.h"
#include "sol_math.h"

void Camera_Tick(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_SolCamera *set = Sol_Comp_Set(world, SolCamera);
    for (int i = 0; i < set->cnt; i++)
    {
        int        id     = set->dense[i];
        SolCamera *camera = &set->data[i];

        SolXform *xform = Sol_Comp_Get(world, id, SolXform);
        SolBody3 *body3 = Sol_Comp_Get(world, id, SolBody3);

        vec3s head = xform->draw_pos;
        head.y += body3->dims.y * 0.8f;
        vec3s lookdir = (vec3s){0, 0, 1.0f};

        if (Sol_Comp_Has(world, id, SolController))
        {
            SolController *cont = Sol_Comp_Get(world, id, SolController);
            lookdir             = cont->lookdir;
        }

        vec3s invDir    = glms_vec3_scale(lookdir, -1.0f);
        vec3s offsetvec = glms_vec3_cross(lookdir, WORLD_UP);

        if (camera->target_distance <= 0)
        {
            camera->target_distance  = 0;
            camera->current_distance = 0;
            camera->anchor           = head;
        }
        else
        {
            // SolRayResult anchortrace =
            //     Sol_Raycast(world, (SolRay){.pos = head, .dir = offsetvec, .dist = camera->target_offset * 2.0f});
            // camera->current_offset   = anchortrace.dist - camera->offset;
            float factor          = 1.0f - expf(-(camera->lerpspeed) * fdt);
            camera->current_offset = Sol_Math_Lerp(camera->current_offset, camera->target_offset, factor);
            camera->anchor        = vecAdd(head, vecSca(offsetvec, camera->current_offset));

            // SolRayResult camDistTrace =
            //     Sol_Raycast(world, (SolRay){.pos = camera->anchor, .dir = invDir, .dist = camera->target_distance
            //     * 1.2f});
            // camera->current_distance = camDistTrace.dist * 0.8f;
            if (camera->target_distance < 0)
                camera->target_distance = 0;
            camera->current_distance = Sol_Math_Lerp(camera->current_distance, camera->target_distance, factor);
        }

        camera->pos = glms_vec3_add(camera->anchor, glms_vec3_scale(invDir, camera->current_distance));
        camera->dir = lookdir;

        // === Wallrun tilt ===
        // float targetRoll = 0.0f;
        // if (world->masks[id] & BITC(HAS_MOVEMENT))
        // {
        //     CompMovement *m = &world->movements[id];
        //     if (m->state == MOVE_WALLRUN)
        //     {
        //         vec3s dir   = vecSub(m->lastTouch, xform->pos);
        //         dir         = vecNorm(dir);
        //         vec3s right = vecCrs(camera->dir, WORLD_UP);
        //         float dot   = vecDot(right, dir);
        //         targetRoll  = -dot * 15.0f * (3.14159f / 180.0f);
        //     }
        // }
        // float rollFactor = 1.0f - expf(-CAMERA_LERP_SPEED * fdt); // slower lerp for cinematic feel
        // camera->roll        = Sol_Math_Lerp(camera->roll, targetRoll, rollFactor);
        // camera->up          = glms_vec3_rotate(WORLD_UP, camera->roll, camera->dir); // rotate up around forward
    }
}

void Camera_Init(World *world)
{
}