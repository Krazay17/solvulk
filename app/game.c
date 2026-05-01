#include "game.h"
// #include <curl/curl.h>

static int player3d;

static void SpawnPlayer(void *data)
{
    World *world = (World *)data;
    if (world->actives[world->playerID])
        Sol_Destroy_Ent(world, world->playerID);

    player3d                         = Sol_Prefab_Wizard(world, (vec3s){0, 5, 0});
    CompController *playerController = Sol_ControllerLocal_Add(world, player3d);
}

struct MakeWiz
{
    World *world;
    u32    amount;
};

void MakeAWizard(void *data)
{
    static int      posInc;
    struct MakeWiz *wizard   = (struct MakeWiz *)data;
    double          time     = Sol_GetGameTime();
    double          epsilonA = sin(time) * 10.0;
    double          epsilonB = cos(time) * 10.0 + 25.0;
    for (int i = 0; i < wizard->amount; i++)
    {
        int id = Sol_Prefab_Wizard(wizard->world, (vec3s){epsilonA, epsilonB, epsilonA});
        Sol_ControllerAi_Add(wizard->world, id);
    }
}

void MakeABox(void *data)
{
    World *world = (World *)data;
    vec3s  pos   = world->controllers[world->playerID].aimpos;
    int    id    = Sol_Prefab_Box(world, pos);
}

void MakeAEmitter(void *data)
{
    World *world = (World *)data;
    vec3s  pos   = world->controllers[world->playerID].aimpos;
    Emitter_Add(world, (EmitterDesc){.emitterKind = EMITTER_FOUNTAIN, .particleKind = PARTICLE_ORB, .pos = pos});
}

void ClearEnts(void *data)
{
    World *world = (World *)data;
    for (int i = world->activeCount; i > 2; i--)
        Sol_Destroy_Ent(world, i);
}

void ColorSpheres(void *data)
{
    World *world = (World *)data;
    Sol_Sphere_ColorAll(world, (vec4s){0, 255, 0, 255});
}

// ─────────────────────────────────────────────────────────────────────────────
// Sol Game Config
// ─────────────────────────────────────────────────────────────────────────────
void Create_Sol_Game()
{
    // CURL *curl = curl_easy_init();
    World *menu = World_Create_Default();
    World *game = World_Create_Default();

    SpawnPlayer(game);
    Sol_Prefab_Floor(game, (vec3s){0, -7, 0});

    int           quitButton     = Sol_Prefab_Button(menu, (vec3s){1000, 30, 0}, "QUIT");
    CompInteract *buttonInteract = Sol_Interact_Add(menu, quitButton);
    buttonInteract->onClick      = (Callback){QuitApp};

    int                   wizOneButton    = Sol_Prefab_Button(menu, (vec3s){10, 250, 0}, "1 Wizard");
    CompInteract         *wizOneButtonInt = Sol_Interact_Add(menu, wizOneButton);
    static struct MakeWiz wizOne          = {0};
    wizOne.amount                         = 1;
    wizOne.world                          = game;
    wizOneButtonInt->onClick              = (Callback){MakeAWizard, &wizOne};

    int                   wizHundredButton = Sol_Prefab_Button(menu, (vec3s){10, 300, 0}, "100 Wizards");
    CompInteract         *interactButton2  = Sol_Interact_Add(menu, wizHundredButton);
    static struct MakeWiz wizhundred       = {0};
    wizhundred.amount                      = 100;
    wizhundred.world                       = game;
    interactButton2->onHold                = (Callback){MakeAWizard, &wizhundred};

    int           button3         = Sol_Prefab_Button(menu, (vec3s){10, 350, 0}, "Spawn Player");
    CompInteract *interactButton3 = Sol_Interact_Add(menu, button3);
    interactButton3->onClick      = (Callback){SpawnPlayer, game};

    int           button4         = Sol_Prefab_Button(menu, (vec3s){10, 400, 0}, "ONTOP");
    CompInteract *interactButton4 = Sol_Interact_Add(menu, button4);
    interactButton4->states |= INTERACT_ISTOGGLE;
    interactButton4->onClick = (Callback){W_Set_Ontop, interactButton4};

    int           buttonClearEnts         = Sol_Prefab_Button(menu, (vec3s){10, 450, 0}, "Clear Ents");
    CompInteract *buttonClearEntsInteract = Sol_Interact_Add(menu, buttonClearEnts);
    buttonClearEntsInteract->onClick      = (Callback){.callbackFunc = ClearEnts, .callbackData = game};

    int           buttonColorSpheres    = Sol_Prefab_Button(menu, (vec3s){10, 500, 0}, "ColorSpheres");
    CompInteract *buttonColorSpheresInt = Sol_Interact_Add(menu, buttonColorSpheres);
    buttonColorSpheresInt->onClick      = (Callback){.callbackFunc = ColorSpheres, .callbackData = game};

    int           fullscreen  = Sol_Prefab_Button(menu, (vec3s){10, 550, 0}, "FullScreen");
    CompInteract *fullscreenI = Sol_Interact_Add(menu, fullscreen);
    fullscreenI->states |= INTERACT_ISTOGGLE;
    fullscreenI->onClick = (Callback){.callbackFunc = W_Set_Fullscreen, .callbackData = fullscreenI};

    int           boxButton  = Sol_Prefab_Button(menu, (vec3s){10, 600, 0}, "MakeABox");
    CompInteract *boxButtonI = Sol_Interact_Add(menu, boxButton);
    boxButtonI->onClick      = (Callback){.callbackFunc = MakeABox, .callbackData = game};

    int           emitterButton  = Sol_Prefab_Button(menu, (vec3s){10, 650, 0}, "MakeAEmitter");
    CompInteract *emitterButtonI = Sol_Interact_Add(menu, emitterButton);
    emitterButtonI->onClick      = (Callback){.callbackFunc = MakeAEmitter, .callbackData = game};
}
