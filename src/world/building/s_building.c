#include "s_building.h"
#include "world.h"
#include "input.h"
#include "model/s_model.h"
#include "physx/s_body.h"
#include "sol/sol.h"

static const u64 required = BITC(HAS_BUILDING);

static void Step(World *world, double dt, double time)
{
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!(world->masks[id] & required))
            continue;
        CompXform    *xform    = &world->xforms[id];
        CompBuilder  *builder  = &world->builders[id];
        CompInteract *interact = &world->interacts[id];
        CompModel    *model    = &world->models[id];
    }
}

static void Tick(World *world, double dt, double time)
{
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!(world->masks[id] & required))
            continue;
        CompBuilder *builder = &world->builders[id];
        SolRayResult result  = Sol_ScreenRaycast(solEngine.activeWorld, (int)Sol_Input_GetMouse().x,
                                                 (int)Sol_Input_GetMouse().y, (SolRay){.dist = 10.0f});

        vec3s gridPos = (vec3s){floorf(result.pos.x), floorf(result.pos.y), floorf(result.pos.z)};
        gridPos       = vecAdd(gridPos, vecSca(result.norm, 0.001f));

        builder->placePos = gridPos;
    }
}

static void Draw(World *world, double dt, double time)
{
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!(world->masks[id] & required))
            continue;
        CompBuilder *builder = &world->builders[id];
        Sol_Render_GetNext_Model(
            builder->model,
            &(ModelSSBO){
                .color    = (vec4s){0, 1, 0, 1},
                .position = (vec4s){builder->placePos.x, builder->placePos.y, builder->placePos.z, 1.0f},
                .rotation = (vec4s){builder->placeRot.x, builder->placeRot.y, builder->placeRot.z, builder->placeRot.w},
                .scale    = (vec4s){1, 1, 1, 1},
            },
            NULL);
    }
}

void Sol_Builder_Init(World *world)
{
    world->builders = calloc(MAX_ENTS, sizeof(CompBuilder));
    WAddTick(world) = Tick;
    WAddStep(world) = Step;
    WAdd3d(world)   = Draw;
}

void Sol_Builder_Add(World *world, int id, u32 model)
{
    CompBuilder *builder = &world->builders[id];
    builder->model       = model;
    builder->placeRot    = glms_quat_identity();

    world->masks[id] |= BITC(HAS_BUILDING);
}

void Sol_Builder_PlaceBuilding(World *world, int id)
{
    CompBuilder *builder = &world->builders[id];
    Sol_Prefab_Building(world, builder->placePos, builder->scale, 0, builder->model);
}

void Sol_Building_RotatePlacing(World *world, int id, float yawDelta)
{
    CompBuilder *builder  = &world->builders[id];
    float        rad      = glm_rad(yawDelta);
    versors      deltaRot = glms_quatv(rad, WORLD_UP);
    builder->placeRot     = glms_quat_mul(builder->placeRot, deltaRot);
}
