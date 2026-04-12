#include "game.h"
#include "sol/sol.h"
#include <curl/curl.h>

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

// ─────────────────────────────────────────────────────────────────────────────
// Sol Game Config
// ─────────────────────────────────────────────────────────────────────────────
void Create_Sol_Game()
{
    CURL *curl = curl_easy_init();
    World *menu = World_Create_Default();
    World *game = World_Create_Default();

    int button = Sol_Prefab_Button(menu, (vec3s){800, 0, 0}, "QUIT");
    Entity_Add_Interact(menu, button, (CompInteractable){.callback = QuitApp});

    int button2 = Sol_Prefab_Button(menu, (vec3s){10, 600, 0}, "MAKE A WIZARD");
    Entity_Add_Interact(menu, button2, (CompInteractable){.callback = MakeAWizard, .callbackData = game, .onHold = true});

    int button3 = Sol_Prefab_Button(menu, (vec3s){0, 660, 0}, "BUTTON");

    int player3d = Sol_Prefab_Wizard(game, (vec3s){0, 0, 0});
    game->playerID = player3d;
    Entity_Add_Controller_Local(game, player3d, (CompController){0});

    // int player2d = Sol_Prefab_Boxman(game, (vec3s){250.0f, -24.0f, 0});
    // Entity_Add_Controller_Local(game, player2d, (CompController){0});
    // static TestData testData = {"TESTING %d", 524};
    // Entity_Add_Interact(game, player2d, (CompInteractable){.callback = TestFunc, .callbackData = &testData});

    int floor = Entity_Create(game);
    Entity_Add_Xform(game, floor, (CompXform){.pos = (vec3s){0, 0, 0}});
    Entity_Add_Model(game, floor, (CompModel){.gpuHandle = SOL_MODEL_WORLD0});

    int floor2 = Entity_Create(game);
    Entity_Add_Xform(game, floor2, (CompXform){.pos = (vec3s){0, -25, 0}, .scale = (vec3s){1, 1, 1}});
    Entity_Add_Model(game, floor2, (CompModel){.gpuHandle = SOL_MODEL_WORLD0});

    // for (int i = 0; i < 1000; i++)
    //     MakeAWizard(game);
}

void MakeAWizard(void *data)
{
    static int posInc;
    World *world = (World *)data;

    int wiz = Sol_Prefab_Wizard(world, (vec3s){0, posInc, 0});
    //Entity_Add_Controller_Local(world, wiz, (CompController){0});
    Entity_Add_Controller_Ai(world, wiz, (CompController){0});
    posInc++;
    Sol_Debug_Add("Entities", world->activeCount);
}