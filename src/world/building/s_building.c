#include "s_building.h"
#include "world.h"
#include "input.h"
#include "model/s_model.h"
#include "physx/s_body.h"
#include "sol/sol.h"

// CompBuilding building_kinds[BUILDINGKIND_COUNT] = {
//     [BUILDINGKIND_WALL] = {
//     },
// };

static const u64 required = BITC(HAS_BUILDING);

static void Step(World *world, double dt, double time)
{
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!(world->masks[id] & required))
            continue;
        CompXform    *xform    = &world->xforms[id];
        CompBuilding *building = &world->buildings[id];
        CompInteract *interact = &world->interacts[id];
        CompModel    *model    = &world->models[id];
        if ((interact->state & INTERACT_CLICKED))
        {
            building->placing = true;
        }
        if (building->placing)
        {
            SolRayResult result = Sol_ScreenRaycast(solEngine.activeWorld, (int)Sol_Input_GetMouseUI().x,
                                                    (int)Sol_Input_GetMouseUI().y, (SolRay){.dist = 10.0f});
            building->placePos  = result.pos;
        }
    }
}

static void Tick(World *world, double dt, double time)
{
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!(world->masks[id] & required))
            continue;
        CompXform    *xform    = &world->xforms[id];
        CompBuilding *building = &world->buildings[id];
        if (building->placing)
        {
            if (Sol_Input_GetMouse().buttonsPressed[SOL_MOUSE_RIGHT])
            {
                building->placing = false;
                continue;
            }
            if (Sol_Input_GetMouse().buttonsPressed[SOL_MOUSE_LEFT])
            {
                Sol_Prefab_Wall3d(solEngine.activeWorld, building->placePos, 1.0f);
            }
        }
    }
}

static void Draw(World *world, double dt, double time)
{
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!(world->masks[id] & required))
            continue;
        CompBuilding *building = &world->buildings[id];
        CompModel    *model    = &world->models[id];
        if (building->placing)
        {
            Sol_Render_GetNext_Model(
                model->modelId,
                &(ModelSSBO){
                    .color    = (vec4s){0, 1, 0, 1},
                    .position = (vec4s){building->placePos.x, building->placePos.y, building->placePos.z, 1.0f},
                    .scale    = (vec4s){1, 1, 1, 1},
                },
                NULL);
        }
    }
}

void Sol_Building_Init(World *world)
{
    world->buildings = calloc(MAX_ENTS, sizeof(CompBuilding));
    WAddTick(world)  = Tick;
    WAddStep(world)  = Step;
    WAdd3d(world)    = Draw;
}

void Sol_Building_Add(World *world, int id, BuildingKind kind)
{
    world->masks[id] |= BITC(HAS_BUILDING);
}
