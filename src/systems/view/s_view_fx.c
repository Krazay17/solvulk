#include "sol_core.h"

static void Fx_Event(World *world, double dt, double time);
static void Fx_Draw(World *world, double dt, double time);

void Sol_View_Fx(World *world)
{
    WAddStep(world) = Fx_Event;
    WAdd3d(world)   = Fx_Draw;
}

static void Fx_Event(World *world, double dt, double time)
{
    for (int i = 0; i < world->events->count; i++)
    {
        SolEvent *e = &world->events->event[i];
        if (e->kind != EVENT_FX)
            continue;
        vec3s pos   = e->as.fx.pos;
        vec3s rot   = e->as.fx.rot;
        float scale = e->as.fx.scale > 0 ? e->as.fx.scale : 0.2f;
        switch (e->as.fx.kind)
        {
        case FXKIND_FIREBALL_HIT:
            Sol_Emitter_Add(world, (Emitter){
                                       .burst = 500,
                                       .pos   = pos,
                                       .particle =
                                           {
                                               .ttl   = 0.4f,
                                               .scale = 0.1f,
                                               .color = {0, 1, 0, .5f},
                                               .kind  = PARTICLE_ORB,
                                               .speed = 14.0f,
                                           },
                                   });
            break;
        case FXKIND_FIRE_APPLY:
            Sol_Emitter_Add(world, (Emitter){
                                       .burst = 80,
                                       .pos   = pos,
                                       .particle =
                                           {
                                               .ttl   = 1.0f,
                                               .scale = 0.4f,
                                               .color = {1, 1, 1, 1},
                                               .kind  = PARTICLE_GFLAME,
                                               .speed = 2.0f,
                                           },
                                   });
            break;
        }
    }
}

static void Fx_Draw(World *world, double dt, double time)
{
}
