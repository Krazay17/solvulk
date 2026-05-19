#include "ability/ability_i.h"
#include "sol_core.h"

void        Sol_View_Ability(World *world)
{
    WAdd3d(world) = Sol_View_Ability_Draw;
}

void Sol_View_Ability_Draw(World *world, double dt, double time)
{
    int required = HAS_ABILITY;
    int count    = world->activeCount;
    for (int i = 0; i < count; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompXform   *xform   = &world->xforms[id];
        CompAbility *ability = &world->abilities[id];

        switch (ability->state)
        {
        case ABILITY_STATE_IDLE:
            break;
        case ABILITY_STATE_DASH:
            break;
        case ABILITY_STATE_CLAW:
            break;
        case ABILITY_STATE_SHIELD:
            Sol_Render_PushSphere((SphereDesc){
                .isfx  = true,
                .color = (vec4s){0.25f, 0.1f, 0.5f, 0.2f},
                .pos   = (vec4s){xform->drawPos.x, xform->drawPos.y, xform->drawPos.z, 1.0f},
            });
            break;
        case ABILITY_STATE_3:
            break;
        case ABILITY_STATE_4:
            break;
        case ABILITY_STATE_5:
            break;
        case ABILITY_STATE_6:
            break;
        case ABILITY_STATE_7:
            break;
        case ABILITY_STATE_8:
            break;
        case ABILITY_STATE_9:
            break;
        }
    }
}