#include "sol_core.h"

#include "ability/ability_i.h"

void Sol_View_Ability(World *world)
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
        case ABILITY_STATE_FIREBALL: {

            AbilityData *data = &world->abilities[id].stateData[world->abilities[id].state];
            if (data->stage > 0)
                break;
            float scale = data->charge * 2.0f + 0.5f;
            vec3s pos   = Sol_Model_GetBoneXform(world, id, "hand.L");
            pos         = vecAdd(pos, vecSca(Sol_Controller_GetAimdir(world, id), scale));
            pos         = vecAdd(pos, vecSca(WORLD_UP, scale));

            SphereSSBO *push = Sol_Render_GetNext_Fireball();
            push->pos[0]     = pos.x;
            push->pos[1]     = pos.y;
            push->pos[2]     = pos.z;
            push->pos[3]     = scale;
            memcpy(push->color, (vec4){1, 0, 0, 0.8f}, sizeof(vec4));
        }
        break;
        case ABILITY_STATE_DASH:
            break;
        case ABILITY_STATE_CLAW:
            break;
        case ABILITY_STATE_SHIELD: {
            SphereSSBO *o = Sol_Render_GetNext_Sphere(true);
            o->pos[0]     = xform->drawPos.x;
            o->pos[1]     = xform->drawPos.y;
            o->pos[2]     = xform->drawPos.z;
            o->pos[3]     = 1.0f;
            o->color[0]   = 0.25f;
            o->color[1]   = 0.1f;
            o->color[2]   = 0.5f;
            o->color[3]   = 0.25f;
        }
        break;
        }
    }
}