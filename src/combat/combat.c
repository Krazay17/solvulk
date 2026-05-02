#include "sol_core.h"

void Combat_Init(World *world)
{
}

CompCombat *Sol_Combat_Add(World *world, int id, CompCombat init)
{
    CompCombat combat  = init;
    world->combats[id] = combat;
    world->masks[id] |= HAS_COMBAT;
    return &world->combats[id];
}

void Combat_Tick(World *world, double dt, double time)
{
    int required = HAS_COMBAT;
    int count    = world->activeCount;
    for (int i = 0; i < count; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompController *controller = &world->controllers[id];
        CompXform      *xform      = &world->xforms[id];
        CompModel      *model      = &world->models[id];
        CompCombat     *combat     = &world->combats[id];

        vec3s aimpos = controller->aimpos;
        vec3s aimdir = controller->aimdir;
        combat->cooldown -= dt;
        if (combat->cooldown <= 0)
            combat->cooldown = 0;

        if (controller->actionState & ACTION_ABILITY1 && combat->cooldown <= 0)
        {
            // SolRayResult result = Sol_RaycastD(world,
            //                                    (SolRay){
            //                                        .pos       = aimpos,
            //                                        .dir       = aimdir,
            //                                        .dist      = 50.0f,
            //                                        .ignoreEnt = id,
            //                                        .mask      = 1,
            //                                    },
            //                                    1.0f);
            // if (result.hit)
            // {
            //     if (result.entId)
            //         Sol_Buff_Add(world, result.entId,
            //                      (CompBuff){.duration = 1.0f,
            //                                 .kind     = BUFF_KNOCKBACK,
            //                                 .hit      = (SolHit){.dir = aimdir, .power = 30.0f}});
            // }

            float min      = 0.2f;
            float max      = 0.8f;
            float randSize = min + (float)rand() / (float)RAND_MAX * (max - min);

            int ball = Sol_Prefab_Ball(
                world, vecAdd(aimpos, vecSca(aimdir, 5.0f)), vecSca(aimdir, 25.0f),
                (CompSphere){.radius = randSize, .color = (vec4s){rand() % 255, rand() % 255, rand() % 255, 255}});

            Sol_Model_PlayAnim(world, id, ANIM_ABILITY0, 6.0f);
            //combat->cooldown = 0.05f;
        }
    }
}
