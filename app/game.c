#include <curl/curl.h>

#include "game.h"

typedef struct
{
    char test[24];
    int testInt;
} TestData;

static void TestFunc(void *data)
{
    TestData *testData = (TestData *)data;
    printf(testData->test, testData->testInt);
}

static int player3d;

static void ResetPlayer(void *data)
{
    World *world = (World *)data;
    CompXform *xform = &world->xforms[player3d];
    xform->pos = (vec3s){0, 5, 0};
}

// ─────────────────────────────────────────────────────────────────────────────
// Sol Game Config
// ─────────────────────────────────────────────────────────────────────────────
void Create_Sol_Game()
{
    CURL *curl = curl_easy_init();
    World *menu = World_Create_Default();
    World *game = World_Create_Default();

    int button = Sol_Prefab_Button(menu, (vec3s){800, 0, 0}, "QUIT");
    CompInteractable *buttonInteract = Entity_Add_Interact(menu, button);
    buttonInteract->callback = QuitApp;

    int button2 = Sol_Prefab_Button(menu, (vec3s){10, 600, 0}, "MAKE A WIZARD");
    CompInteractable *interactButton2 = Entity_Add_Interact(menu, button2);
    interactButton2->callback = MakeAWizard;
    interactButton2->callbackData = game;
    interactButton2->onHold = true;

    int button3 = Sol_Prefab_Button(menu, (vec3s){10, 500, 0}, "RESET PLAYER");
    CompInteractable *interactButton3 = Entity_Add_Interact(menu, button3);
    interactButton3->callback = ResetPlayer;
    interactButton3->callbackData = game;

    player3d = Sol_Prefab_Wizard(game, (vec3s){0, 5, 0});
    Entity_Add_Controller_Local(game, player3d);

    int floor = Entity_Create(game);
    CompXform *floorXform = Entity_Add_Xform(game, floor, (vec3s){100, 0, 0});
    CompModel *floorModel = Entity_Add_Model(game, floor, SOL_MODEL_WORLD1);
    CompBody *floorBody = Entity_Add_Body3(game, floor, (CompBody){.shape = SHAPE3_MOD});
}

void MakeAWizard(void *data)
{
    static int posInc;
    World *world = (World *)data;
    double time = Sol_GetGameTime();
    double epsilonA = sin(time) * 10.0;
    double epsilonB = cos(time) * 10.0 + 25.0;
    for (int i = 0; i < 10; i++)
    {
        int wiz = Sol_Prefab_Wizard(world, (vec3s){
                                               epsilonA,
                                               epsilonB,
                                               epsilonA,
                                           });
        Entity_Add_Controller_Ai(world, wiz);
    }
    Sol_Debug_Add("Entities", Sol_World_GetEntCount(world));
}