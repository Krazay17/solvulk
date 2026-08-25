#include "game.h"

static int player2d;
static int hudPlayerHealth, hudPlayerEnergy;

static void SpawnPlayer(int flags)
{
    Sol_Destroy_Ent(Sol_GetActiveGameWorld(), Sol_Player_GetEnt(Sol_GetActiveGameWorld(), 0));

    int id = Sol_Prefab_Factory(
        Sol_GetActiveGameWorld(), 0, EKIND_PLAYER,
        (EntDesc){.pos = Sol_GetActiveGameWorld()->playerSpawns[0], .scale = 1.0f, .authority = NETAUTH_AUTH});
    Sol_Movement_Add(Sol_GetActiveGameWorld(), id, MOVEMENTKIND_PLAYER);
    Sol_Cam_Add(Sol_GetActiveGameWorld(), id, CAMKIND_3D, true);
    Sol_Player_Add(Sol_GetActiveGameWorld(), id, 0);
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

void SpawnWizard(int flags)
{
    int    id;
    double time     = solState.gameTime;
    double epsilonA = 0.0;  // sin(time) * 10.0;
    double epsilonB = 10.0; // cos(time) * 10.0 + 25.0;
    id              = Sol_Prefab_Wizard(Sol_GetActiveGameWorld(), 0, (vec3s){epsilonA, epsilonB, epsilonA}, 1.0f);
}

void SpawnZorgon(int flags)
{
    int    id;
    double time     = solState.gameTime;
    double epsilonA = 0.0;  // sin(time) * 10.0;
    double epsilonB = 10.0; // cos(time) * 10.0 + 25.0;
    id              = Sol_Prefab_Zorgon(Sol_GetActiveGameWorld(), 0, (vec3s){epsilonA, epsilonB, epsilonA}, 2.0f);
}

void MakeABox(int flags)
{
    vec3s pos =
        Sol_Controller_GetShootPos(Sol_GetActiveGameWorld(), Sol_Player_GetEnt(Sol_GetActiveGameWorld(), 0), 1.0f);
    Sol_Prefab_Box(Sol_GetActiveGameWorld(), pos);
}

void ClearEnts(int flags)
{
    World *world = Sol_GetActiveGameWorld();
    if (world)
        for (int i = world->activeCount - 1; i >= 0; i--)
        {
            int id = world->activeEntities[i];
            // if (WHas(world, id, BITC(HAS_REPLICATION)) && world->replications[id].auth == NETAUTH_AUTH)
            //     continue;
            Sol_Destroy_Ent(world, id);
        }
}

void ColorSpheres(int flags)
{
    World *world = Sol_GetActiveGameWorld();
    Sol_Shape_ColorAll(world, (vec4s){0, 255, 0, 255});
}

void HostGame(int flags)
{
    Net_Connect(true, "127.0.0.1", 8080);

    for (int k = -4; k < 4; k++)
    {
        int id = Sol_Prefab_Factory(
            Sol_GetActiveGameWorld(), 0, EKIND_WIZARD,
            (EntDesc){.pos = (vec3s){k * 4.0f, 10.0f, 60.0f}, .scale = 1.0f, .authority = NETAUTH_AUTH});
        // if (id)
        //     Sol_Ai_Add(Sol_GetActiveGameWorld(), id, AIKIND_WIZARD);
    }
}

void ClientConnect(int flags)
{
    ClearEnts(0);
    if (flags > 0)
        Net_Connect(false, "127.0.0.1", 8080);
    else
        Net_Connect(false, "answer-cuba.gl.at.ply.gg", 35101);
}

void Disconnect(int flags)
{
    Net_Disconnect();
    Sol_Replication_Disconnect(Sol_GetActiveGameWorld());
}

void RotateGuy(World *world, double dt, double time)
{
    static float yaw = 0;
    yaw += dt;

    world->xforms[player2d].quat = Sol_Quat_FromYawPitch(yaw, 0);
}

void SaveGame(void)
{
    Sol_User_SaveUserSettings(0);
}

void Spectate(int flags)
{
    int             playerId   = Sol_Player_GetEnt(Sol_GetActiveGameWorld(), 0);
    CompController *controller = Sol_Controller_Get(Sol_GetActiveGameWorld(), playerId);
    float           yaw        = 0;
    vec3s           pos        = Sol_Xform_GetPos(Sol_GetActiveGameWorld(), playerId);
    if (controller)
        yaw = controller->yaw;

    Sol_Destroy_Ent(Sol_GetActiveGameWorld(), Sol_Player_GetEnt(Sol_GetActiveGameWorld(), 0));
    int id = Sol_Prefab_Spectate(Sol_GetActiveGameWorld(), pos, yaw);
}

static float GetPlayerHealth(World *world, int id)
{
    int playerId = Sol_Player_GetEnt(world, 0);
    if (playerId < 0)
        return 0.0f;
    return world->combats[playerId].health;
}
static float GetPlayerHealthMax(World *world, int id)
{
    int playerId = Sol_Player_GetEnt(world, 0);

    if (playerId < 0)
        return 0.0f;
    return world->combats[playerId].maxHealth;
}
static float GetPlayerEnergy(World *world, int id)
{
    int playerId = Sol_Player_GetEnt(world, 0);
    if (playerId < 0)
        return 0.0f;
    return world->combats[playerId].energy;
}
static float GetPlayerEnergyMax(World *world, int id)
{
    int playerId = Sol_Player_GetEnt(world, 0);
    if (playerId < 0)
        return 0.0f;
    return world->combats[playerId].maxEnergy;
}

// ─────────────────────────────────────────────────────────────────────────────
// Sol Game App
// ─────────────────────────────────────────────────────────────────────────────
void Create_Sol_Game()
{
    World_Create_All();
    Sol_SetActiveGameWorld(WORLDID_GAME3D_1);
    Sol_GetWorldById(WORLDID_GAME3D_1)->doesSimulate = true;
    Sol_GetWorldById(WORLDID_GAME3D_1)->doesRender   = true;

    // Sol_GetWorldById(WORLDID_SETTINGS)   = World_Create_Default(WORLDKIND_MENU);
    // Sol_GetWorldById(WORLDID_HUD)    = World_Create_Default(WORLDKIND_GAME2D);
    // Sol_GetActiveGameWorld() = World_Create_Default(WORLDKIND_GAME);
    // Sol_GetActiveGameWorld() = Sol_GetActiveGameWorld();

    // Sol_GetActiveGameWorld() = Sol_GetActiveGameWorld();
    // Sol_World_SetReplicates(Sol_GetActiveGameWorld(), true);

    WAdd2d(Sol_GetWorldById(WORLDID_HUD)) = Sol_Crosshair_Draw;

    // player2d = Sol_Prefab_Dude2d(Sol_GetWorldById(WORLDID_HUD), (vec3s){1100.0f, 400.0f, 1.0f}, 1.0f);
    // Sol_Player_Add(Sol_GetWorldById(WORLDID_HUD), player2d, 0);
    // Sol_Movement_Add(Sol_GetWorldById(WORLDID_HUD), player2d, MOVEMENTKIND_PLAYER);
    // WAddStep(Sol_GetWorldById(WORLDID_HUD)) = RotateGuy;

    hudPlayerHealth = Sol_Prefab_EnergyBar(
        Sol_GetWorldById(WORLDID_HUD), (vec3s){515, 600, 0}, (vec4s){0.0f, 1.0f, 0.0f, 1.0f}, Sol_GetActiveGameWorld(),
        Sol_Player_GetEnt(Sol_GetActiveGameWorld(), 0), GetPlayerHealth, GetPlayerHealthMax);
    hudPlayerEnergy = Sol_Prefab_EnergyBar(
        Sol_GetWorldById(WORLDID_HUD), (vec3s){515, 620, 0}, (vec4s){1.0f, 1.0f, 0.0f, 1.0f}, Sol_GetActiveGameWorld(),
        Sol_Player_GetEnt(Sol_GetActiveGameWorld(), 0), GetPlayerEnergy, GetPlayerEnergyMax);

    Sol_Prefab_Building_Button(Sol_GetWorldById(WORLDID_HUD), (vec3s){1000, 200.0f, 1.0f}, 1.0f, MODELKIND_WALL);
    Sol_Prefab_Building_Button(Sol_GetWorldById(WORLDID_HUD), (vec3s){1000, 200.0f, 1.0f}, 1.0f, MODELKIND_FLOOR);

    int attackBar = Sol_Create_Ent(Sol_GetWorldById(WORLDID_HUD), 0);
    Sol_Body2d_Add(Sol_GetWorldById(WORLDID_HUD), attackBar, BODY2DKIND_RECT, 140.0f, 70.0f, 0);
    Sol_Xform_Set(Sol_GetWorldById(WORLDID_HUD), attackBar, 340, 650, 0);
    Sol_View2d_Add(Sol_GetWorldById(WORLDID_HUD), attackBar, VIEW2DKIND_RECT, (vec4s){1.0f, 1.0f, 1.0f, 1.0f}, 140, 70);
    Sol_Interact_Add(Sol_GetWorldById(WORLDID_HUD), attackBar);

    for (int i = 0; i < 2; i++)
    {
        char *label = i == 0 ? "L" : "R";
        int   slot  = Sol_Prefab_AbilitySlot(Sol_GetWorldById(WORLDID_HUD), (vec3s){1.0f, 1.0f, 1.0f}, i, label);
        Sol_Parent_Set(Sol_GetWorldById(WORLDID_HUD), slot,
                       (CompParent){.active = true, .parentId = attackBar, .localOffset = {70.0f * i}});
    }

    int dashAbility = Sol_Prefab_AbilitySlot(Sol_GetWorldById(WORLDID_HUD), (vec3s){800, 650, 0}, 9, "Shift");
    Sol_Interact_Add(Sol_GetWorldById(WORLDID_HUD), dashAbility);

    int abilityBar = Sol_Create_Ent(Sol_GetWorldById(WORLDID_HUD), 0);
    Sol_Body2d_Add(Sol_GetWorldById(WORLDID_HUD), abilityBar, BODY2DKIND_RECT, 280, 70, 0);
    Sol_Xform_Set(Sol_GetWorldById(WORLDID_HUD), abilityBar, 500, 650, 0);
    Sol_View2d_Add(Sol_GetWorldById(WORLDID_HUD), abilityBar, VIEW2DKIND_RECT, (vec4s){1.0f, 1.0f, 1.0f, 1.0f}, 280,
                   70);
    Sol_Interact_Add(Sol_GetWorldById(WORLDID_HUD), abilityBar);

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
        int abilitySlot =
            Sol_Prefab_AbilitySlot(Sol_GetWorldById(WORLDID_HUD), (vec3s){1.0f, 1.0f, 1.0f}, i + 2, label);
        Sol_Parent_Set(Sol_GetWorldById(WORLDID_HUD), abilitySlot,
                       (CompParent){.active = true, .parentId = abilityBar, .localOffset = {70.0f * i}});
    }
    // Left and right
    Sol_Prefab_AbilityCard(Sol_GetWorldById(WORLDID_HUD), (vec3s){340, 650}, ABILITY_STATE_CLAW, 2);
    Sol_Prefab_AbilityCard(Sol_GetWorldById(WORLDID_HUD), (vec3s){410, 650}, ABILITY_STATE_FIREBALL, 2);
    // // 1,2,3,4
    // Sol_Prefab_AbilityCard(Sol_GetWorldById(WORLDID_HUD), (vec3s){500, 650}, ABILITY_STATE_SPINSLASH, 2);
    // Sol_Prefab_AbilityCard(Sol_GetWorldById(WORLDID_HUD), (vec3s){570, 650}, ABILITY_STATE_SPINSLASH, 2);
    // Sol_Prefab_AbilityCard(Sol_GetWorldById(WORLDID_HUD), (vec3s){640, 650}, ABILITY_STATE_SHIELD, 2);
    // Sol_Prefab_AbilityCard(Sol_GetWorldById(WORLDID_HUD), (vec3s){710, 650}, ABILITY_STATE_SHIELD, 2);
    // // Shift
    // Sol_Prefab_AbilityCard(Sol_GetWorldById(WORLDID_HUD), (vec3s){800, 650, 0}, ABILITY_STATE_DASH, 2);
    // Sol_Prefab_AbilityCard(Sol_GetWorldById(WORLDID_HUD), (vec3s){920, 500, 0}, ABILITY_STATE_SPINSLASH, 3);

    // for (int i = 1; i < 8; i++)
    // {
    //     float yoffset = 70.0f * (float)i;
    //     for (int j = 0; j < 3; j++)
    //     {
    //         float xoffset = 70.0f * (float)j;
    //         Sol_Prefab_AbilityCard(Sol_GetWorldById(WORLDID_HUD), (vec3s){xoffset, yoffset, 0}, i, j);
    //     }
    // }

    Sol_Prefab_Clouds(Sol_GetActiveGameWorld(), (vec3s){0, 0, 0});

    int quitButton = Sol_Prefab_Button(Sol_GetWorldById(WORLDID_SETTINGS), (vec3s){1000, 30, 0}, "QUIT");
    Sol_Interact_Set(Sol_GetWorldById(WORLDID_SETTINGS), quitButton,
                     (CompInteract){.onClick = (SolCallback){.callbackFunc = QuitApp}});

    int wizOneButton = Sol_Prefab_Button(Sol_GetWorldById(WORLDID_SETTINGS), (vec3s){10, 250, 0}, "Spawn 1Wizard");
    Sol_Interact_Set(Sol_GetWorldById(WORLDID_SETTINGS), wizOneButton,
                     (CompInteract){.onClick = (SolCallback){.callbackFunc = SpawnWizard}});
    CompSlider *wizOneSlider    = Sol_Slider_Add(Sol_GetWorldById(WORLDID_SETTINGS), wizOneButton);
    wizOneSlider->handle_width  = 40.0f;
    wizOneSlider->handle_height = 80.0f;

    int wizHundredButton = Sol_Prefab_Button(Sol_GetWorldById(WORLDID_SETTINGS), (vec3s){10, 300, 0}, "Spawn Wizards");
    Sol_Interact_Set(Sol_GetWorldById(WORLDID_SETTINGS), wizHundredButton,
                     (CompInteract){.onHold = (SolCallback){.callbackFunc = SpawnWizard}});

    int spawnZorgonButton =
        Sol_Prefab_Button(Sol_GetWorldById(WORLDID_SETTINGS), (vec3s){160, 300, 0}, "Spawn Zorgons");
    Sol_Interact_Set(Sol_GetWorldById(WORLDID_SETTINGS), spawnZorgonButton,
                     (CompInteract){.onHold = (SolCallback){SpawnZorgon}});

    int saveButton = Sol_Prefab_Button(Sol_GetWorldById(WORLDID_SETTINGS), (vec3s){200, 350, 0}, "Save");
    Sol_Interact_Set(Sol_GetWorldById(WORLDID_SETTINGS), saveButton,
                     (CompInteract){.onClick = (SolCallback){.callbackFunc = Sol_User_SaveUserSettings}});

    int button3 = Sol_Prefab_Button(Sol_GetWorldById(WORLDID_SETTINGS), (vec3s){10, 350, 0}, "Spawn Player");
    Sol_Interact_Set(Sol_GetWorldById(WORLDID_SETTINGS), button3,
                     (CompInteract){.onClick = (SolCallback){SpawnPlayer}});

    int button4 = Sol_Prefab_Button(Sol_GetWorldById(WORLDID_SETTINGS), (vec3s){10, 400, 0}, "ONTOP");
    Sol_Interact_Set(Sol_GetWorldById(WORLDID_SETTINGS), button4,
                     (CompInteract){.state   = INTERACT_TOGGLEABLE,
                                    .onClick = (SolCallback){
                                        .callbackFunc = W_Set_Ontop,
                                        .flag = Sol_Interact_Get(Sol_GetWorldById(WORLDID_SETTINGS), button4)->state}});

    int buttonClearEnts = Sol_Prefab_Button(Sol_GetWorldById(WORLDID_SETTINGS), (vec3s){10, 450, 0}, "Clear Ents");
    Sol_Interact_Set(Sol_GetWorldById(WORLDID_SETTINGS), buttonClearEnts,
                     (CompInteract){.onClick = (SolCallback){.callbackFunc = ClearEnts}});

    int buttonColorSpheres = Sol_Prefab_Button(Sol_GetWorldById(WORLDID_SETTINGS), (vec3s){10, 500, 0}, "ColorSpheres");
    Sol_Interact_Set(Sol_GetWorldById(WORLDID_SETTINGS), buttonColorSpheres,
                     (CompInteract){.onClick = (SolCallback){.callbackFunc = ColorSpheres}});

    int fullscreen = Sol_Prefab_Button(Sol_GetWorldById(WORLDID_SETTINGS), (vec3s){10, 550, 0}, "FullScreen");
    Sol_Interact_Set(Sol_GetWorldById(WORLDID_SETTINGS), fullscreen,
                     (CompInteract){.state   = INTERACT_TOGGLEABLE,
                                    .onClick = (SolCallback){
                                        .callbackFunc = W_Set_Fullscreen,
                                        .flag = Sol_Interact_Get(Sol_GetWorldById(WORLDID_HUD), fullscreen)->state}});

    int boxButton = Sol_Prefab_Button(Sol_GetWorldById(WORLDID_SETTINGS), (vec3s){10, 600, 0}, "MakeABox");
    Sol_Interact_Set(Sol_GetWorldById(WORLDID_SETTINGS), boxButton,
                     (CompInteract){.onClick = (SolCallback){.callbackFunc = MakeABox}});

    int testButton = Sol_Prefab_Button(Sol_GetWorldById(WORLDID_SETTINGS), (vec3s){10, 650, 0}, "Test");
    Sol_Interact_Add(Sol_GetWorldById(WORLDID_SETTINGS), testButton);

    int hostButton = Sol_Prefab_Button(Sol_GetWorldById(WORLDID_SETTINGS), (vec3s){1130, 150, 0}, "Host");
    Sol_Interact_Set(Sol_GetWorldById(WORLDID_SETTINGS), hostButton,
                     (CompInteract){.onClick = (SolCallback){.callbackFunc = HostGame}});

    int connectButton = Sol_Prefab_Button(Sol_GetWorldById(WORLDID_SETTINGS), (vec3s){1130, 200, 0}, "Connect");
    Sol_Interact_Set(Sol_GetWorldById(WORLDID_SETTINGS), connectButton,
                     (CompInteract){.onClick = (SolCallback){.callbackFunc = ClientConnect}});

    int connectButtonLocal =
        Sol_Prefab_Button(Sol_GetWorldById(WORLDID_SETTINGS), (vec3s){1130, 250, 0}, "ConnectLocal");
    Sol_Interact_Set(Sol_GetWorldById(WORLDID_SETTINGS), connectButtonLocal,
                     (CompInteract){.onClick = (SolCallback){.callbackFunc = ClientConnect, .flag = 1}});

    int disconnectButton = Sol_Prefab_Button(Sol_GetWorldById(WORLDID_SETTINGS), (vec3s){1130, 300, 0}, "Disconnect");
    Sol_Interact_Set(Sol_GetWorldById(WORLDID_SETTINGS), disconnectButton,
                     (CompInteract){.onClick = (SolCallback){.callbackFunc = Disconnect}});

    int world1Button = Sol_Prefab_Button(Sol_GetWorldById(WORLDID_SETTINGS), (vec3s){1130, 500, 0}, "World1");

    int debugButton = Sol_Prefab_Button(Sol_GetWorldById(WORLDID_SETTINGS), (vec3s){1130, 600, 0}, "Debug");
    Sol_Interact_Set(Sol_GetWorldById(WORLDID_SETTINGS), debugButton,
                     (CompInteract){.onClick = (SolCallback){.callbackFunc = Sol_ToggleDebug}});

    int spectateButton = Sol_Prefab_Button(Sol_GetWorldById(WORLDID_SETTINGS), (vec3s){1130, 650, 0}, "Spectate");
    Sol_Interact_Set(Sol_GetWorldById(WORLDID_SETTINGS), spectateButton,
                     (CompInteract){.onClick = (SolCallback){.callbackFunc = Spectate}});
}
