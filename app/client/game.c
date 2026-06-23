#include "game.h"

static World *gameWorld;
static World *gameWorld2;
static int    player2d;

static void SpawnPlayer(int flags, void *data)
{
    Sol_Destroy_Ent(gameWorld, gameWorld->playerID);

    Sol_Prefab_Factory(gameWorld, 1, EKIND_PLAYER,
                       (EntDesc){.pos = (vec3s){0, 5, 0}, .scale = 1.0f, .authority = NETAUTH_AUTH});
    //    gameWorld->models[1].modelId = MODELKIND_ZORGON;
    Sol_Controller_Add(gameWorld, 1, CONTROLLER_LOCAL);
    // RibbonHandle handle = Sol_Ribbon_Add(gameWorld, 1, RIBBONKIND_LIGHTNING, 1.0f, (vec4s){1, 1, 1, 1});
    // Sol_Ribbon_UpdateTargetPos(gameWorld, handle, (vec3s){0, 25, 25});
    // Sol_Ribbon_AddBetweenEntities(gameWorld, 1, 2, RIBBONKIND_LIGHTNING, 1.0f, (vec4s){1, 1, 1, 1});
}

typedef enum
{
    ENEMYKIND_WIZARD,
    ENEMYKIND_ZORGON,
} EnemyKind;
struct MakeEnemy
{
    World *world;
    u32    enemyKind;
};
void SpawnEnemy(int flags, void *data)
{
    if (Net_IsClient())
        return;
    static int        posInc;
    struct MakeEnemy *enemy    = (struct MakeEnemy *)data;
    double            time     = solState.gameTime;
    double            epsilonA = sin(time) * 10.0;
    double            epsilonB = cos(time) * 10.0 + 25.0;

    switch (enemy->enemyKind)
    {
    case ENEMYKIND_WIZARD: {

        int id = Sol_Prefab_Wizard(gameWorld, 0, (vec3s){epsilonA, epsilonB, epsilonA}, 1.0f);
        Sol_Ai_Add(gameWorld, id, AICONTROLLERKIND_WIZARD);
        Sol_Replication_Add(gameWorld, id, NETAUTH_AUTH, EKIND_WIZARD);
    }
    break;
    case ENEMYKIND_ZORGON: {

        int id = Sol_Prefab_Zorgon(gameWorld, 0, (vec3s){epsilonA, epsilonB, epsilonA}, 2.0f);
        Sol_Ai_Add(gameWorld, id, AICONTROLLERKIND_WIZARD);
        Sol_Replication_Add(gameWorld, id, NETAUTH_AUTH, EKIND_WIZARD);
    }
    break;
    }
}

void MakeABox(int flags, void *data)
{
    World *world = (World *)data;
    vec3s  pos   = Sol_Controller_GetAimPos(world, 1);
    Sol_Prefab_Box(world, pos);
}

void ClearEnts(int flags, void *data)
{
    World *world = (World *)data;
    if (world)
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
            gameWorld, 0, EKIND_WIZARD,
            (EntDesc){.pos = (vec3s){k * 4.0f, 10.0f, 60.0f}, .scale = 1.0f, .authority = NETAUTH_AUTH});
        if (id)
            Sol_Ai_Add(gameWorld, id, AICONTROLLERKIND_WIZARD);
    }
}

void ClientConnect(int flags, void *data)
{
    ClearEnts(0, gameWorld);
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

void RotateGuy(World *world, double dt, double time)
{
    static float yaw = 0;
    yaw += dt;

    world->xforms[player2d].quat = Sol_Quat_FromYawPitch(yaw, 0);
}

void WizSpawner(World *world, double dt)
{
    static float            accum  = 0;
    static u32              amount = 3;
    static struct MakeEnemy enemy  = {0};
    enemy.world                    = world;
    enemy.enemyKind                = ENEMYKIND_WIZARD;

    accum += dt;

    if (accum > 1.0f)
    {
        accum = 0;
        for (int i = 0; i < amount; i++)
        {
            SpawnEnemy(0, &enemy);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Sol Game App
// ─────────────────────────────────────────────────────────────────────────────
void Create_Sol_Game()
{
    gameWorld   = World_Create_Default(WORLDKIND_GAME);
    World *hud  = World_Create_Default(WORLDKIND_MENU);
    World *menu = World_Create_Default(WORLDKIND_MENU);

    Sol_World_SetActive(gameWorld);
    Sol_World_SetReplicates(gameWorld, true);
    // Sol_View_Crosshair(gameWorld);

    SpawnPlayer(0, 0);
    int floorWorld1 = Sol_Create_Ent(gameWorld, 0);
    Sol_Xform_Teleport(gameWorld, floorWorld1, (vec3s){0, -7, 0});
    Sol_Model_Add(gameWorld, floorWorld1, SOL_MODEL_WORLD1, 0);
    Sol_Body_Add(gameWorld, floorWorld1, (BodyDesc){.shape = SHAPE3_MOD});
    // WAddStep(gameWorld) = WizSpawner;

    // int blade = Sol_Create_Ent(gameWorld, 0);
    // Sol_Xform_Add(gameWorld, blade, (vec3s){0,0,0});
    // Sol_Xform_SetScale(gameWorld, blade, (vec3s){20.0f, 20.0f, 20.0f});
    // Sol_Model_Add(gameWorld, blade, MODELKIND_WEAPONBLADE, 0);
    // Sol_Body_Add(gameWorld, blade, (BodyDesc){.mass = 0, .shape = SHAPE3_MOD});

    Sol_Dmgnumbers_Spawn(gameWorld, 0, 25, (vec3s){0,1,0});

    player2d                 = Sol_Create_Ent(hud, 0);
    CompModel *player2dModel = Sol_Model_Add(hud, player2d, MODELKIND_DUDE, -300.0f);
    Sol_Xform_Add(hud, player2d, (vec3s){1100.0f, 400.0f, 0.0f});
    Sol_Xform_SetScale(hud, player2d, (vec3s){75.0f, 75.0f, 75.0f});
    player2dModel->is2d    = true;
    player2dModel->xOffset = 50.0f;

    WAddStep(hud)            = RotateGuy;
    CompBody2d *player2dBody = Sol_Body2d_Add(hud, player2d, BODY2DKIND_RECT, 100.0f, 150.0f, 0b01, 0b01);
    player2dBody->grav       = (vec2s){0, 9.0f};
    Sol_Interact_Add(hud, player2d);

    Sol_Prefab_Healthbar(hud, (vec3s){515, 600, 0}, gameWorld, 1);

    int leftAbility = Sol_Prefab_AbilitySlot(hud, (vec3s){390, 600, 0}, 0, "Q");
    Sol_Interact_Add(hud, leftAbility);
    int rightAbility = Sol_Prefab_AbilitySlot(hud, (vec3s){820, 600, 0}, 1, "E");
    Sol_Interact_Add(hud, rightAbility);

    int dashAbility = Sol_Prefab_AbilitySlot(hud, (vec3s){920, 600, 0}, 9, "Shift");
    Sol_Interact_Add(hud, dashAbility);

    int abilityBar = Sol_Create_Ent(hud, 0);
    Sol_Body2d_Add(hud, abilityBar, BODY2DKIND_RECT, 280, 70, 0, 0);
    Sol_Xform_Set(hud, abilityBar, 500, 650, 0);
    Sol_View2d_Add(hud, abilityBar, VIEW2DKIND_RECT, (vec4s){0.0f, 0.0f, 0.0f, 1.0f}, 280, 70);
    Sol_Interact_Add(hud, abilityBar);

    for (int i = 0; i < 4; i++)
    {
        char label[8];
        switch (i)
        {
        case 0:
            snprintf(label, sizeof(label), "1");
            break;
        case 1:
            snprintf(label, sizeof(label), "2");
            break;
        case 2:
            snprintf(label, sizeof(label), "3");
            break;
        case 3:
            snprintf(label, sizeof(label), "4");
            break;
        }
        int abilitySlot = Sol_Prefab_AbilitySlot(hud, (vec3s){1.0f, 1.0f, 1.0f}, i + 2, label);
        Sol_Parent_Set(hud, abilitySlot,
                       (CompParent){.active = true, .parentId = abilityBar, .localOffset = {70.0f * i}});
    }
    // Left and right
    Sol_Prefab_AbilityCard(hud, (vec3s){390, 600}, ABILITY_STATE_CLAW, 2);
    Sol_Prefab_AbilityCard(hud, (vec3s){820, 600}, ABILITY_STATE_FIREBALL, 2);
    // 1,2,3,4
    Sol_Prefab_AbilityCard(hud, (vec3s){500, 650}, ABILITY_STATE_SPINSLASH, 2);
    Sol_Prefab_AbilityCard(hud, (vec3s){570, 650}, ABILITY_STATE_SPINSLASH, 2);
    Sol_Prefab_AbilityCard(hud, (vec3s){640, 650}, ABILITY_STATE_SHIELD, 2);
    Sol_Prefab_AbilityCard(hud, (vec3s){710, 650}, ABILITY_STATE_SHIELD, 2);
    // Shift
    Sol_Prefab_AbilityCard(hud, (vec3s){920, 600, 0}, ABILITY_STATE_DASH, 2);
    Sol_Prefab_AbilityCard(hud, (vec3s){920, 500, 0}, ABILITY_STATE_SPINSLASH, 3);

    for (int i = 1; i < 8; i++)
    {
        float yoffset = 70.0f * (float)i;
        for (int j = 0; j < 3; j++)
        {
            float xoffset = 70.0f * (float)j;
            Sol_Prefab_AbilityCard(hud, (vec3s){xoffset, yoffset, 0}, i, j);
        }
    }

    Sol_Prefab_Clouds(gameWorld, (vec3s){0, 0, 0});

    int quitButton = Sol_Prefab_Button(menu, (vec3s){1000, 30, 0}, "QUIT");
    Sol_Interact_Set(menu, quitButton, (CompInteract){.onClick = (Callback){QuitApp}});

    int                     wizOneButton = Sol_Prefab_Button(menu, (vec3s){10, 250, 0}, "Spawn 1Wizard");
    static struct MakeEnemy wizOne       = {0};
    wizOne.world                         = gameWorld;
    wizOne.enemyKind                     = ENEMYKIND_WIZARD;
    Sol_Interact_Set(menu, wizOneButton, (CompInteract){.onClick = (Callback){SpawnEnemy, &wizOne}});

    int                     wizHundredButton = Sol_Prefab_Button(menu, (vec3s){10, 300, 0}, "Spawn Wizards");
    static struct MakeEnemy wizhundred       = {0};
    wizhundred.world                         = gameWorld;
    wizhundred.enemyKind                     = ENEMYKIND_WIZARD;
    Sol_Interact_Set(menu, wizHundredButton, (CompInteract){.onHold = (Callback){SpawnEnemy, &wizhundred}});

    int                     spawnZorgonButton = Sol_Prefab_Button(menu, (vec3s){160, 300, 0}, "Spawn Zorgons");
    static struct MakeEnemy makeEnemyZorgon   = {0};
    makeEnemyZorgon.world                     = gameWorld;
    makeEnemyZorgon.enemyKind                 = ENEMYKIND_ZORGON;
    Sol_Interact_Set(menu, spawnZorgonButton, (CompInteract){.onHold = (Callback){SpawnEnemy, &makeEnemyZorgon}});

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

    int testButton = Sol_Prefab_Button(menu, (vec3s){10, 650, 0}, "Test");
    Sol_Interact_Add(menu, testButton);

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
}
