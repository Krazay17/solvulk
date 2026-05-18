#include "game.h"
// #include <curl/curl.h>

static int    player3d;
static World *gameWorld;

static void SpawnPlayer(int flags, void *data)
{
    if (gameWorld->actives[gameWorld->playerID])
        Sol_Destroy_Ent(gameWorld, gameWorld->playerID);

    player3d = Sol_Prefab_Player(gameWorld, (vec3s){0, 5, 0}, 1.0f);
    Sol_Controller_Add(gameWorld, player3d, (ControllerDesc){.kind = CONTROLLER_LOCAL});
    Sol_Buff_Add(gameWorld, player3d, (BuffDesc){.duration = 2.0f, .kind = BUFFKIND_FIRE, .freq = 0.5f}, NULL);
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
        int id = Sol_Prefab_Wizard(gameWorld, (vec3s){epsilonA, epsilonB, epsilonA}, 1.0f);
        Sol_Controller_Add(wizard->world, id, (ControllerDesc){.kind = CONTROLLER_AI});
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
    //    Sol_Event_Add(world, (SolEvent){.kind = EVENT_FX, .as.fx.kind=FXKIND_FIREBALL_HIT, .as.fx.pos=pos});
    Sol_Emitter_Add(world, (Emitter){.pos      = pos,
                                     .burst    = 1000,
                                     .inf      = 1,
                                     .rate = 0.1f,
                                     .particle = (Particle){
                                         .ttl      = 120.0f,
                                         .color    = (vec4s){.r = 1, .g = 1, .b = 1, .a = 0.2f},
                                         .scale    = 40.0f,
                                         .kind     = PARTICLE_CLOUD,
                                         .rotspeed = Sol_RandRange(-.2f, .2f),
                                         .speed    = 1.0f,
                                         .offset   = 100.0f,
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

// ─────────────────────────────────────────────────────────────────────────────
// Sol Game App
// ─────────────────────────────────────────────────────────────────────────────
void Create_Sol_Game()
{
    // CURL *curl = curl_easy_init();
    World *menu = World_Create_Default();
    gameWorld   = World_Create_Default();
    Sol_View_Crosshair(gameWorld);

    // Sol_Audio_Beep();
    // Sol_Audio_Play(SOL_AUDIO_MENUMUSIC);
    
    SpawnPlayer(0, 0);
    Sol_Prefab_Floor(gameWorld, (vec3s){0, -7, 0});
    MakeAEmitter(0, gameWorld);

    int floor2 = Sol_Create_Ent(gameWorld);
    Sol_Xform_Add(gameWorld, floor2, (vec3s){0, 25, 0});
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
    wizhundred.amount                      = 100;
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
}
