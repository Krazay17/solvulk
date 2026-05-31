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
            push->pos = (vec4s){pos.x, pos.y, pos.z, scale};
            push->color = (vec4s){1, 0, 0, 0.8f};
        }
        break;
        case ABILITY_STATE_DASH:
            break;
        case ABILITY_STATE_CLAW:
            break;
        case ABILITY_STATE_SHIELD: {
            SphereSSBO *o = Sol_Render_GetNext_Sphere(true);
            o->pos = (vec4s){xform->drawPos.x,xform->drawPos.y,xform->drawPos.z,1.0f};
            o->color = (vec4s){0.25f, 0.1f, 0.5f, 0.25f};
        }
        break;
        }
    }
}