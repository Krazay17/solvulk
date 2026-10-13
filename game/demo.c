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

    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "QUIT", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_Quit);
    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "Debug", INTERACTFLAG_DRAGGABLE | INTERACTFLAG_TOGGLEABLE, UILAYER_4,
                      Hook_DebugToggle);
    Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "FULLSCREEN", INTERACTFLAG_DRAGGABLE | INTERACTFLAG_TOGGLEABLE,
                      UILAYER_4, Hook_Fullscreen);
    Sol_Prefab_Slider(world, Sol_Grid_Next(&grid), "Volume", INTERACTFLAG_DRAGGABLE, UILAYER_4, Hook_SetVolume);
    int chill_button = Sol_Prefab_Button(world, Sol_Grid_Next(&grid), "Chill",
                                         INTERACTFLAG_DRAGGABLE | INTERACTFLAG_TOGGLEABLE, UILAYER_4, Hook_Chill);
    forc(world, chill_button, ScInteract) c->state |= (1u << INTERACT_ACTIVE);
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

    Sol_User_AddItem(&(SolItem){.item_kind = ITEMKIND_BOLT});
    Sol_User_AddItem(&(SolItem){.item_kind = ITEMKIND_BOLT});
    Sol_User_AddItem(&(SolItem){.item_kind = ITEMKIND_CLAW});
    Sol_User_AddItem(&(SolItem){.item_kind = ITEMKIND_CLAW});
    Sol_User_AddItem(&(SolItem){.item_kind = ITEMKIND_FIREBALL});
    Sol_User_AddItem(&(SolItem){.item_kind = ITEMKIND_FIREBALL});
}

static void Heal_Kill_Handler(World *world, SolEvent *e)
{
    forc(world, e->dealer, ScCombat)
    {
        c->health = c->healthMax;
    }
    Sol_Comp_Rem(world, e->dealer, ScBuff);
}

void Create_Game()
{
    World *world        = World_Create_AllSys();
    sol_user.game_world = world->index;
    Sol_Event_AddHandler(world, EVENTKIND_DEATH, Heal_Kill_Handler);

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