#include "game.h"

void Create_Sol_Game()
{
    Create_Menu();
    Create_Game();
    Create_Hud();
}

void Create_Menu()
{
    World *world        = World_Create();
    sol_user.menu_world = world->index;
    Sol_Sys_Add(world, WORLDSYS_INTERACT);
    Sol_Sys_Add(world, WORLDSYS_HOOK);
    Sol_Sys_Add(world, WORLDSYS_BODY2);
    Sol_Sys_Add(world, WORLDSYS_VIEW2);
    GridMaker grid = {.cols = 1, .start = (vec2s){1130.0f, 100.0f}, .spacing = (vec2s){120.0f, 60.0f}};

    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "QUIT", INTERACT_DRAGGABLE, UILAYER_4, Hook_Quit);
    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "Debug", INTERACT_DRAGGABLE | INTERACT_TOGGLEABLE, UILAYER_4,
                      Hook_DebugToggle);
    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "FULLSCREEN", INTERACT_DRAGGABLE | INTERACT_TOGGLEABLE, UILAYER_4,
                      Hook_Fullscreen);
    Sol_Prefab_Slider(world, Sol_Grid_Next(&grid), "Volume", INTERACT_DRAGGABLE, UILAYER_4, Hook_SetVolume);
    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "Chill", INTERACT_DRAGGABLE | INTERACT_TOGGLEABLE | INTERACT_TOGGLED,
                      UILAYER_4, Hook_Chill);
}

void Create_Hud()
{
    World *world       = World_Create();
    sol_user.hud_world = world->index;
    Sol_Sys_Add(world, WORLDSYS_INTERACT);
    Sol_Sys_Add(world, WORLDSYS_HOOK);
    Sol_Sys_Add(world, WORLDSYS_BODY2);
    Sol_Sys_Add(world, WORLDSYS_VIEW2);
    Sol_Sys_Add(world, WORLDSYS_ABILITYBAR);
    Sol_Sys_Add(world, WORLDSYS_REF);

    Sol_Prefab_Crosshair(world);

    Sol_User_AddItem(&(SolItem){.abilityKind = ABILITYKIND_BOLT});
    Sol_User_AddItem(&(SolItem){.abilityKind = ABILITYKIND_BOLT});
    Sol_User_AddItem(&(SolItem){.abilityKind = ABILITYKIND_CLAW});
    Sol_User_AddItem(&(SolItem){.abilityKind = ABILITYKIND_CLAW});
    Sol_User_AddItem(&(SolItem){.abilityKind = ABILITYKIND_FIREBALL});
    Sol_User_AddItem(&(SolItem){.abilityKind = ABILITYKIND_FIREBALL});
}

static void Heal_On_Kill(World *world, double dt)
{
    forc(world, 0, SlEvent)
    {
        int count = solb_count(c->events);
        sollog(count);
        for (int i = 0; i < count; i++)
        {
            SolEvent *e = &c->events[i];
            if (e->kind != EVENTKIND_DEATH)
                continue;
            forc(world, e->entA, ScCombat)
            {
                c->health = c->healthMax;
            }
            Sol_Comp_Rem(world, e->entA, ScBuff);
        }
    }
}
void Create_Game()
{
    World *world        = World_Create_AllSys();
    sol_user.game_world = world->index;
    WAddPosttick(world)     = Heal_On_Kill;
    { // Player
        int id            = Sol_Prefab_Dude(world, (vec3s){0, 6, -5}, 1.0f);
        sol_user.view_ent = id;
        Sol_Comp_Add(world, id, ScPlayer);
        ScMeta *meta = Sol_Comp_Add(world, id, ScMeta);
        snprintf(meta->name, sizeof(meta->name), "Player");
        Sol_Debug_Add("Player Ent", (float)id);
    }
    { // Ai Dude
        vec3s spawn_pos      = {0, 5, 10};
        int id               = Sol_Prefab_Dude(world, spawn_pos, 1.0f);
        ScAi *ai             = Sol_Comp_Add(world, id, ScAi);
        ai->aggroRange       = 100.0f;
        ai->inactive         = true;
        ScCombat *combat     = Sol_Comp_Add(world, id, ScCombat);
        combat->respawnTime  = 2.0f;
        world->xform.rot[id] = glms_euler_xyz_quat((vec3s){0.0f, glm_rad(180.0f), 0.0f});
    }
    { // Level
        int level1          = Sol_Create_Ent(world, (vec3s){0, 0, 0});
        ScModel *levelModel = Sol_Comp_Add(world, level1, ScModel);
        levelModel->kind    = MODELKIND_WORLD10;
        ScStage *stage      = Sol_Comp_Add(world, level1, ScStage);
        stage->isDirty      = true;
    }
}