#include "game.h"

static World *gameWorld;
static World *gameWorld2;

static void SpawnPlayer(int flags, void *data)
{
    Sol_Destroy_Ent(gameWorld, gameWorld->playerID);

    Sol_Prefab_Factory(gameWorld, 1, ENTKIND_PLAYER,
                       (EntDesc){.pos = (vec3s){0, 5, 0}, .scale = 1.0f, .authority = NETAUTH_AUTH});
    Sol_Controller_Add(gameWorld, 1, CONTROLLER_LOCAL);
}

struct MakeWiz
{
    World *world;
    u32    amount;
};
void MakeAWizard(int flags, void *data)
{
    if (Net_IsClient())
        return;
    static int      posInc;
    struct MakeWiz *wizard   = (struct MakeWiz *)data;
    double          time     = Sol_GetGameTime();
    double          epsilonA = sin(time) * 10.0;
    double          epsilonB = cos(time) * 10.0 + 25.0;
    for (int i = 0; i < wizard->amount; i++)
    {
        int id = Sol_Prefab_Wizard(gameWorld, 0, (vec3s){epsilonA, epsilonB, epsilonA}, 1.0f);
        Sol_AiController_Add(gameWorld, id, AICONTROLLERKIND_WIZARD);
        Sol_Replication_Add(gameWorld, id, NETAUTH_AUTH, ENTKIND_WIZARD);
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
    Sol_Emitter_SpawnEx(world, (Emitter){.burst    = 100,
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
    Net_Connect(true, "127.0.0.1", 8080);

    for (int k = -4; k < 4; k++)
    {
        int id = Sol_Prefab_Factory(
            gameWorld, 0, ENTKIND_WIZARD,
            (EntDesc){.pos = (vec3s){k * 4.0f, 10.0f, 60.0f}, .scale = 1.0f, .authority = NETAUTH_AUTH});
        if (id)
            Sol_AiController_Add(gameWorld, id, AICONTROLLERKIND_WIZARD);
    }
}

void ClientConnect(int flags, void *data)
{
    if (data)
        Net_Connect(false, "127.0.0.1", 8080);
    else
        Net_Connect(false, "answer-cuba.gl.at.ply.gg", 35101);
}

void Disconnect(int flags, void *data)
{
    Net_Disconnect();
    Sol_Replication_Disconnect(gameWorld);
}

void ChangeWorld(int flags, void *data)
{
    u32 idx = *(u32 *)data;
    switch (idx)
    {
    case 0:
        break;
    case 1:
        break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Sol Game App
// ─────────────────────────────────────────────────────────────────────────────
void Create_Sol_Game()
{
    World *menu = World_Create_Default(WORLDKIND_MENU);

    gameWorld = World_Create_Default(WORLDKIND_GAME);
    Sol_World_SetActive(gameWorld);
    Sol_World_SetReplicates(gameWorld, true);
    Sol_View_Crosshair(gameWorld);

    SpawnPlayer(0, 0);

    World *hud = World_Create_Default(WORLDKIND_MENU);
    Sol_Prefab_Healthbar(hud, (vec3s){515, 600, 0}, gameWorld, 1);
    Sol_Prefab_AbilitySlot(hud, (vec3s){200, 700}, 0);
    Sol_Prefab_AbilitySlot(hud, (vec3s){300, 700}, 1);
    Sol_Prefab_AbilitySlot(hud, (vec3s){400, 700}, 2);
    Sol_Prefab_AbilitySlot(hud, (vec3s){500, 700}, 3);
    Sol_Prefab_AbilityCard(hud, (vec3s){200, 200}, ABILITY_STATE_FIREBALL);
    Sol_Prefab_AbilityCard(hud, (vec3s){200, 300}, ABILITY_STATE_PISTOL);
    Sol_Prefab_AbilityCard(hud, (vec3s){200, 400}, ABILITY_STATE_SHIELD);


    int floorWorld1 = Sol_Create_Ent(gameWorld, 0);
    Sol_Xform_Teleport(gameWorld, floorWorld1, (vec3s){0, -7, 0});
    Sol_Model_Add(gameWorld, floorWorld1, (ModelDesc){.id = SOL_MODEL_WORLD1});
    Sol_Body_Add(gameWorld, floorWorld1, (BodyDesc){.shape = SHAPE3_MOD});

    Sol_Prefab_Clouds(gameWorld, (vec3s){0, 0, 0});

    int quitButton = Sol_Prefab_Button(menu, (vec3s){1000, 30, 0}, "QUIT");
    Sol_Interact_Set(menu, quitButton, (CompInteract){.onClick = (Callback){QuitApp}});

    int                   wizOneButton = Sol_Prefab_Button(menu, (vec3s){10, 250, 0}, "1 Wizard");
    static struct MakeWiz wizOne       = {0};
    wizOne.amount                      = 1;
    wizOne.world                       = gameWorld;
    Sol_Interact_Set(menu, wizOneButton, (CompInteract){.onClick = (Callback){MakeAWizard, &wizOne}});

    int                   wizHundredButton = Sol_Prefab_Button(menu, (vec3s){10, 300, 0}, "100 Wizards");
    static struct MakeWiz wizhundred       = {0};
    wizhundred.amount                      = 1;
    wizhundred.world                       = gameWorld;
    Sol_Interact_Set(menu, wizHundredButton, (CompInteract){.onHold = (Callback){MakeAWizard, &wizhundred}});

    int button3 = Sol_Prefab_Button(menu, (vec3s){10, 350, 0}, "Spawn Player");
    Sol_Interact_Set(menu, button3, (CompInteract){.onClick = (Callback){SpawnPlayer, gameWorld}});

    int button4 = Sol_Prefab_Button(menu, (vec3s){10, 400, 0}, "ONTOP");
    Sol_Interact_Set(menu, button4, (CompInteract){.state = INTERACT_TOGGLEABLE, .onClick = (Callback){W_Set_Ontop}});

    int buttonClearEnts = Sol_Prefab_Button(menu, (vec3s){10, 450, 0}, "Clear Ents");
    Sol_Interact_Set(menu, buttonClearEnts,
                     (CompInteract){.onClick = (Callback){.callbackFunc = ClearEnts, .callbackData = gameWorld}});

    int buttonColorSpheres = Sol_Prefab_Button(menu, (vec3s){10, 500, 0}, "ColorSpheres");
    Sol_Interact_Set(menu, buttonColorSpheres,
                     (CompInteract){.onClick = (Callback){.callbackFunc = ColorSpheres, .callbackData = gameWorld}});

    int fullscreen = Sol_Prefab_Button(menu, (vec3s){10, 550, 0}, "FullScreen");
    Sol_Interact_Set(
        menu, fullscreen,
        (CompInteract){.state = INTERACT_TOGGLEABLE, .onClick = (Callback){.callbackFunc = W_Set_Fullscreen}});

    int boxButton = Sol_Prefab_Button(menu, (vec3s){10, 600, 0}, "MakeABox");
    Sol_Interact_Set(menu, boxButton,
                     (CompInteract){.onClick = (Callback){.callbackFunc = MakeABox, .callbackData = gameWorld}});

    int emitterButton = Sol_Prefab_Button(menu, (vec3s){10, 650, 0}, "MakeAEmitter");
    Sol_Interact_Set(menu, emitterButton,
                     (CompInteract){.onHold = (Callback){.callbackFunc = MakeAEmitter, .callbackData = gameWorld}});

    int hostButton = Sol_Prefab_Button(menu, (vec3s){1130, 150, 0}, "Host");
    Sol_Interact_Set(menu, hostButton, (CompInteract){.onClick = (Callback){.callbackFunc = HostGame}});

    int connectButton = Sol_Prefab_Button(menu, (vec3s){1130, 200, 0}, "Connect");
    Sol_Interact_Set(menu, connectButton, (CompInteract){.onClick = (Callback){.callbackFunc = ClientConnect}});

    int connectButtonLocal = Sol_Prefab_Button(menu, (vec3s){1130, 250, 0}, "ConnectLocal");
    Sol_Interact_Set(menu, connectButtonLocal,
                     (CompInteract){.onClick = (Callback){.callbackFunc = ClientConnect, .callbackData = (void *)1}});

    int disconnectButton = Sol_Prefab_Button(menu, (vec3s){1130, 300, 0}, "Disconnect");
    Sol_Interact_Set(menu, disconnectButton, (CompInteract){.onClick = (Callback){.callbackFunc = Disconnect}});

    int world1Button = Sol_Prefab_Button(menu, (vec3s){1130, 500, 0}, "World1");
    Sol_Interact_Set(menu, world1Button,
                     (CompInteract){.onClick = (Callback){.callbackFunc = ChangeWorld, .callbackData = gameWorld}});

    // int world2Button = Sol_Prefab_Button(menu, (vec3s){1130, 550, 0}, "World2");
    // Sol_Interact_Set(menu, world2Button,
    //                  (InteractDesc){.onClick = (Callback){.callbackFunc = Disconnect, .callbackData = gameWorld2}});
}
