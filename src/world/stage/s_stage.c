#include "s_stage.h"
#include "sol_core.h"
#include "sol_math.h"
#include "world.h"
#include "physx/physx_i.h"
#include "model.h"
#include "render/render.h"
#include "model.h"
#include "game/prefabs.h"

void Stage_Draw(World *world, double dt, double time)
{
    Stage *stage = world->stage;
    if (!stage)
        return;
    Sol_Render_GetNext_Model(stage->handle,
                             &(ModelSSBO){
                                 .position = stage->pos,
                                 .scale    = stage->scale,
                             },
                             NULL);
}

void Sol_Stage_Init(World *world)
{
    Stage *stage = calloc(1, sizeof(Stage));
    if (!stage)
    {
        fprintf(stderr, "Failed to allocate memory for stage\n");
        return;
    }
    world->stage  = stage;
    WAdd3d(world) = Stage_Draw;

    stage->handle = SOL_MODEL_WORLD9;
    stage->scale  = (vec4s){1.0f, 1.0f, 1.0f, 1.0f};
    Sol_Stage_Load(world, stage, "", (vec3s){0, 0, 0});
}

bool Sol_Stage_Load(World *world, Stage *stage, const char *path, vec3s offset)
{
    if (!stage)
        return false;
    PhysxGroup *group = &world->spatial->staticGroup;

    u32 modelTriCount = Sol_Model_GetTriCount(stage->handle);
    u32 oldCount      = group->triCount;
    u32 newCount      = oldCount + modelTriCount;

    if (newCount > group->capacity)
    {
        group->capacity = newCount * 2;
        group->tris     = realloc(group->tris, sizeof(SolTri) * group->capacity);
    }

    group->triCount = newCount;

    // Add_Static_Collision_From_Model(world, 0, group);
    SolModel *model = &loaded_models[stage->handle];

    for (int i = 0; i < model->tri_count; i++)
    {
        group->tris[oldCount + i] = model->tris[i];
    }
    for (int i = 0; i < model->prefab_count; i++)
    {
        const char *name = model->prefabs[i].name;
        if (strncmp(name, "PREFAB_PLAYERSPAWN", 11) == 0)
        {
            world->playerSpawns[world->playerSpawnCount++] = model->prefabs[i].pos;
        }
        if (strncmp(name, "PREFAB_WIZARD", 13) == 0)
        {
            Sol_Prefab_Wizard(world, 0, model->prefabs[i].pos, 1.0f);
        }
        if (strncmp(name, "PREFAB_ZORGON", 13) == 0)
        {
            Sol_Prefab_Zorgon(world, 0, model->prefabs[i].pos, 1.0f);
        }
    }

    return true;
}

void Sol_Stage_Unload(World *world, Stage *stage)
{
}
