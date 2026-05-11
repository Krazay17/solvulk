#include "game.h"
// #include <curl/curl.h>

static int    player3d;
static World *gameWorld;

static void SpawnPlayer(int flags, void *data)
{
    if (gameWorld->actives[gameWorld->playerID])
        Sol_Destroy_Ent(gameWorld, gameWorld->playerID);

    player3d = Sol_Prefab_Player(gameWorld, (vec3s){0,5,0}, 1.0f);
    Sol_Controller_Add(gameWorld, player3d, (ControllerDesc){.kind = CONTROLLER_LOCAL});
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
    Emitter_Add(world,
                (Emitter){.pos      = pos,
                          .ttl      = 50.0f,
                          .rate     = 0.1f,
                          .burst    = 50,
                          .vel      = (vec3s){0, 0, 0},
                          .particle = (Particle){
                              .ttl = 5.0f, .color = (vec4s){.r = 255, .g = 0, .b = 55, .a = 255}, .scale = 0.15f}});
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
    Sol_Sphere_ColorAll(world, (vec4s){0, 255, 0, 255});
}

// ─────────────────────────────────────────────────────────────────────────────
// Sol Game App
// ─────────────────────────────────────────────────────────────────────────────
void Create_Sol_Game()
{
    // CURL *curl = curl_easy_init();
    World *menu = World_Create_Default();
    gameWorld   = World_Create_Default();

    // Sol_PlayAudio(SOL_AUDIO_MENUMUSIC);

    SpawnPlayer(0, 0);
    Sol_Prefab_Floor(gameWorld, (vec3s){0, -7, 0});

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
    Sol_Interact_Add(menu, button4, (InteractDesc){.states = INTERACT_TOGGLEABLE, .onClick = (Callback){W_Set_Ontop}});

    int buttonClearEnts = Sol_Prefab_Button(menu, (vec3s){10, 450, 0}, "Clear Ents");
    Sol_Interact_Add(menu, buttonClearEnts,
                     (InteractDesc){.onClick = (Callback){.callbackFunc = ClearEnts, .callbackData = gameWorld}});

    int buttonColorSpheres = Sol_Prefab_Button(menu, (vec3s){10, 500, 0}, "ColorSpheres");
    Sol_Interact_Add(menu, buttonColorSpheres,
                     (InteractDesc){.onClick = (Callback){.callbackFunc = ColorSpheres, .callbackData = gameWorld}});

    int fullscreen = Sol_Prefab_Button(menu, (vec3s){10, 550, 0}, "FullScreen");
    Sol_Interact_Add(
        menu, fullscreen,
        (InteractDesc){.states = INTERACT_TOGGLEABLE, .onClick = (Callback){.callbackFunc = W_Set_Fullscreen}});

    int boxButton = Sol_Prefab_Button(menu, (vec3s){10, 600, 0}, "MakeABox");
    Sol_Interact_Add(menu, boxButton,
                     (InteractDesc){.onClick = (Callback){.callbackFunc = MakeABox, .callbackData = gameWorld}});

    int emitterButton = Sol_Prefab_Button(menu, (vec3s){10, 650, 0}, "MakeAEmitter");
    Sol_Interact_Add(menu, emitterButton,
                     (InteractDesc){.onHold = (Callback){.callbackFunc = MakeAEmitter, .callbackData = gameWorld}});

                     
}
