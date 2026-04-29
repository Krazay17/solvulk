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

void KillLastEnt(void *data)
{
    World *world = (World *)data;
    for (int i = world->activeCount; i >= 2; i--)
        Sol_Destroy_Ent(world, i);

    Sol_Debug_Add("Entities", Sol_World_GetEntCount(world));
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

    int               quitButton     = Sol_Prefab_Button(menu, (vec3s){1000, 30, 0}, "QUIT");
    CompInteractable *buttonInteract = Sol_Interact_Add(menu, quitButton);
    buttonInteract->callback         = (Callback){QuitApp};

    int                   wizOneButton    = Sol_Prefab_Button(menu, (vec3s){10, 300, 0}, "1 Wizard");
    CompInteractable     *interactButton5 = Sol_Interact_Add(menu, wizOneButton);
    static struct MakeWiz wizOne          = {0};
    wizOne.amount                         = 1;
    wizOne.world                          = game;
    interactButton5->callback             = (Callback){MakeAWizard, &wizOne};

    int                   wizHundredButton = Sol_Prefab_Button(menu, (vec3s){10, 400, 0}, "100 Wizards");
    CompInteractable     *interactButton2  = Sol_Interact_Add(menu, wizHundredButton);
    static struct MakeWiz wizhundred       = {0};
    wizhundred.amount                      = 100;
    wizhundred.world                       = game;
    interactButton2->callback              = (Callback){MakeAWizard, &wizhundred};
    interactButton2->onHold                = true;

    int               button3         = Sol_Prefab_Button(menu, (vec3s){10, 500, 0}, "Spawn Player");
    CompInteractable *interactButton3 = Sol_Interact_Add(menu, button3);
    interactButton3->callback         = (Callback){SpawnPlayer, game};

    int               button4         = Sol_Prefab_Button(menu, (vec3s){10, 600, 0}, "ONTOP");
    CompInteractable *interactButton4 = Sol_Interact_Add(menu, button4);
    interactButton4->callback         = (Callback){W_Set_Ontop};

    int               buttonKillLastEnt         = Sol_Prefab_Button(menu, (vec3s){10, 700, 0}, "Clear Ents");
    CompInteractable *buttonKillLastEntInteract = Sol_Interact_Add(menu, buttonKillLastEnt);
    buttonKillLastEntInteract->callback         = (Callback){.callbackFunc = KillLastEnt, .callbackData = game};

    int               buttonColorSpheres    = Sol_Prefab_Button(menu, (vec3s){1000, 300, 0}, "ColorSpheres");
    CompInteractable *buttonColorSpheresInt = Sol_Interact_Add(menu, buttonColorSpheres);
    buttonColorSpheresInt->callback         = (Callback){.callbackFunc = ColorSpheres, .callbackData = game};

    int        floor      = Sol_Create_Ent(game);
    CompXform *floorXform = Sol_Xform_Add(game, floor, (vec3s){0, 0, 0});
    CompModel *floorModel = Sol_Model_Add(game, floor, (CompModel){.modelId = SOL_MODEL_WORLD1});
    CompBody  *floorBody  = Sol_Physx_Add(game, floor, (CompBody){.shape = SHAPE3_MOD});
}
