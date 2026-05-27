#include "game.h"

#define WIZARD_SPAWN_SPACING 4.0f

static World *gameWorld;

static void SpawnPlayer(int flags, void *data)
{
    Sol_Destroy_Ent(gameWorld, gameWorld->playerID);

    Sol_Prefab_Factory(gameWorld, 1, PREFABKIND_PLAYER,
                       (PrefabDesc){.pos = (vec3s){0, 5, 0}, .scale = 1.0f, .authority = NETAUTH_AUTH});
    Sol_Controller_Add(gameWorld, 1, CONTROLLER_LOCAL);
    Sol_Owner_Add(gameWorld, 1, (OwnerDesc){.team = 0});
}

struct MakeWiz
{
    World *world;
    u32    amount;
};
void MakeAWizard(int flags, void *data)
{
    static int      posInc;
    struct MakeWiz *wizard   = (struct MakeWiz *)data;
    double          time     = Sol_GetGameTime();
    double          epsilonA = sin(time) * 10.0;
    double          epsilonB = cos(time) * 10.0 + 25.0;
    for (int i = 0; i < wizard->amount; i++)
    {
        int id = Sol_Prefab_Wizard(gameWorld, 0, (vec3s){epsilonA, epsilonB, epsilonA}, 1.0f);
        Sol_AiController_Add(gameWorld, id, AICONTROLLERKIND_WIZARD);
        Sol_Replication_Add(gameWorld, id, NETAUTH_AUTH, PREFABKIND_WIZARD);
    }
}

void MakeABox(int flags, void *data)
{
    World *world = (World *)data;
    vec3s  pos   = Sol_Controller_GetAimPos(world, world->playerID);
    Sol_Prefab_Box(world, pos);
}

void MakeAEmitter(int flags, void *data)
{
    World *world = (World *)data;
    vec3s  pos   = Sol_Controller_GetAimPos(world, world->playerID);
    Sol_Event_Add(world, (SolEvent){.kind = EVENTKIND_FX, .as.fx.kind = FXKIND_FIREBALL_HIT, .as.fx.pos = pos});
    Sol_Emitter_Add(world, (Emitter){.burst    = 100,
                                     .rate     = 0.1f,
                                     .pos      = pos,
                                     .inf      = 1,
                                     .particle = {
                                         .speed     = 5.0f,
                                         .ttl       = 5.0f,
                                         .randScale = 1,
                                         .kind      = PARTICLE_SHOCK,
                                         .rot       = Sol_Math_RandRange2(-2.0f, 2.0f),
                                         .rotspeed  = 3.0f,
                                         .scalein   = 0.2f,
                                     }});
}

void ClearEnts(int flags, void *data)
{
    World *world = (World *)data;
    for (int i = world->activeCount; i > 2; i--)
        Sol_Destroy_Ent(world, i);
}

void ColorSpheres(int flags, void *data)
{
    World *world = (World *)data;
    Sol_Shape_ColorAll(world, (vec4s){0, 255, 0, 255});
}

void HostGame(int flags, void *data)
{
    Net_Init(&Sol_GetState()->netEngine, true, "127.0.0.1", 8080);

    for (int k = -4; k < 4; k++)
    {
        int id =
            Sol_Prefab_Factory(gameWorld, 0, PREFABKIND_WIZARD,
                               (PrefabDesc){.pos = (vec3s){k * WIZARD_SPAWN_SPACING, 10.0f, 60.0f}, .scale = 1.0f});
        // int id = Sol_Prefab_Wizard(gameWorld, 0, (vec3s){k * WIZARD_SPAWN_SPACING, 10.0f, 60.0f}, 1.0f);
        Sol_AiController_Add(gameWorld, id, AICONTROLLERKIND_WIZARD);
    }
}

void ClientConnect(int flags, void *data)
{
    Net_Init(&Sol_GetState()->netEngine, false, "127.0.0.1", 8080);
}

// ─────────────────────────────────────────────────────────────────────────────
// Sol Game App
// ─────────────────────────────────────────────────────────────────────────────
void Create_Sol_Game()
{
    World *menu = World_Create_Default(WORLDKIND_MENU);
    // Sol_World_SetActive(menu, true);

    gameWorld = World_Create_Default(WORLDKIND_GAME);
    Sol_State_SetPlayerWorld(gameWorld);
    Sol_World_SetReplicates(gameWorld, true);
    Sol_Cam_SetActivecam(gameWorld);
    Sol_View_Crosshair(gameWorld);
    // Sol_Input_SetLocked(true);

    SpawnPlayer(0, 0);
    Sol_Prefab_Floor(gameWorld, (vec3s){0, -7, 0});

    // for (int i = -5; i < 5; i++)
    // {
    //     for (int j = -5; j < 5; j++)
    //     {
    //         int id = Sol_Prefab_Wizard(gameWorld, (vec3s){i * spacing, 10.0f, (j * spacing) - 60.0f}, 1.0f);
    //         Sol_AiController_Add(gameWorld, id, (AiControllerDesc){0});
    //     }
    // }

    Sol_Render_SkyboxSet(gameWorld, SOL_TEXTURE_REDSKY);
    Sol_Prefab_Clouds(gameWorld, (vec3s){0, 0, 0});

    int floor2 = Sol_Create_Ent(gameWorld, 0);
    Sol_Xform_Add(gameWorld, floor2, (vec3s){5, 25, 0});
    Sol_Model_Add(gameWorld, floor2, (ModelDesc){.id = SOL_MODEL_BOX});
    Sol_Body_Add(gameWorld, floor2,
                 (BodyDesc){.shape = SHAPE3_SPH, .mass = 100.0f, .radius = 1.0f, .group = 0b11, .restitution = 0.1f});
    Sol_Interact_Add(gameWorld, floor2, (InteractDesc){0});
    Sol_Flags_Add(gameWorld, floor2, EFLAG_PICKUPABLE);

    int quitButton = Sol_Prefab_Button(menu, (vec3s){1000, 30, 0}, "QUIT");
    Sol_Interact_Add(menu, quitButton, (InteractDesc){.onClick = (Callback){QuitApp}});

    int                   wizOneButton = Sol_Prefab_Button(menu, (vec3s){10, 250, 0}, "1 Wizard");
    static struct MakeWiz wizOne       = {0};
    wizOne.amount                      = 1;
    wizOne.world                       = gameWorld;
    Sol_Interact_Add(menu, wizOneButton, (InteractDesc){.onClick = (Callback){MakeAWizard, &wizOne}});

    int                   wizHundredButton = Sol_Prefab_Button(menu, (vec3s){10, 300, 0}, "100 Wizards");
    static struct MakeWiz wizhundred       = {0};
    wizhundred.amount                      = 1;
    wizhundred.world                       = gameWorld;
    Sol_Interact_Add(menu, wizHundredButton, (InteractDesc){.onHold = (Callback){MakeAWizard, &wizhundred}});

    int button3 = Sol_Prefab_Button(menu, (vec3s){10, 350, 0}, "Spawn Player");
    Sol_Interact_Add(menu, button3, (InteractDesc){.onClick = (Callback){SpawnPlayer, gameWorld}});

    int button4 = Sol_Prefab_Button(menu, (vec3s){10, 400, 0}, "ONTOP");
    Sol_Interact_Add(menu, button4, (InteractDesc){.toggleable = true, .onClick = (Callback){W_Set_Ontop}});

    int buttonClearEnts = Sol_Prefab_Button(menu, (vec3s){10, 450, 0}, "Clear Ents");
    Sol_Interact_Add(menu, buttonClearEnts,
                     (InteractDesc){.onClick = (Callback){.callbackFunc = ClearEnts, .callbackData = gameWorld}});

    int buttonColorSpheres = Sol_Prefab_Button(menu, (vec3s){10, 500, 0}, "ColorSpheres");
    Sol_Interact_Add(menu, buttonColorSpheres,
                     (InteractDesc){.onClick = (Callback){.callbackFunc = ColorSpheres, .callbackData = gameWorld}});

    int fullscreen = Sol_Prefab_Button(menu, (vec3s){10, 550, 0}, "FullScreen");
    Sol_Interact_Add(menu, fullscreen,
                     (InteractDesc){.toggleable = true, .onClick = (Callback){.callbackFunc = W_Set_Fullscreen}});

    int boxButton = Sol_Prefab_Button(menu, (vec3s){10, 600, 0}, "MakeABox");
    Sol_Interact_Add(menu, boxButton,
                     (InteractDesc){.onClick = (Callback){.callbackFunc = MakeABox, .callbackData = gameWorld}});

    int emitterButton = Sol_Prefab_Button(menu, (vec3s){10, 650, 0}, "MakeAEmitter");
    Sol_Interact_Add(menu, emitterButton,
                     (InteractDesc){.onHold = (Callback){.callbackFunc = MakeAEmitter, .callbackData = gameWorld}});

    int hostButton = Sol_Prefab_Button(menu, (vec3s){1130, 150, 0}, "Host");
    Sol_Interact_Add(menu, hostButton, (InteractDesc){.onClick = (Callback){.callbackFunc = HostGame}});

    int connectButton = Sol_Prefab_Button(menu, (vec3s){1130, 200, 0}, "Connect");
    Sol_Interact_Add(menu, connectButton, (InteractDesc){.onClick = (Callback){.callbackFunc = ClientConnect}});
}
