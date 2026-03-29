#include "game.h"
#include "sol.h"

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
    World *menu = World_Create_Default();
    World *game = World_Create_Default();

    int button = Sol_Prefab_Button(menu, (vec3s){800, 0, 0}, "QUIT");
    Entity_Add_Interact(menu, button, (CompInteractable){.callback = QuitApp});

    int button2 = Sol_Prefab_Button(menu, (vec3s){10, 600, 0}, "MAKE A WIZARD");
    Entity_Add_Interact(menu, button2, (CompInteractable){.callback = MakeAWizard, .callbackData = menu});

    int player3d = Sol_Prefab_Wizard(game, (vec3s){0, 0, 0});
    Entity_Add_Controller_Local(game, player3d, (CompController){0});

    int player2d = Sol_Prefab_Boxman(game, (vec3s){250.0f, -24.0f, 0});
    Entity_Add_Controller_Local(game, player2d, (CompController){0});
    static TestData testData = {"TESTING %d", 524};
    Entity_Add_Interact(game, player2d, (CompInteractable){.callback = TestFunc, .callbackData = &testData});

    int floor = Entity_Create(game);
    Entity_Add_Xform(game, floor, (CompXform){.pos = (vec3s){0, 0, 0}});
    Entity_Add_Model(game, floor, (CompModel){.gpuHandle = Sol_Loader_GetBank()->models.world0});

    int floor2 = Entity_Create(game);
    Entity_Add_Xform(game, floor2, (CompXform){.pos = (vec3s){0, -25, 0}, .scale = (vec3s){5, 2, 25}});
    Entity_Add_Model(game, floor2, (CompModel){.gpuHandle = Sol_Loader_GetBank()->models.world0});
}


void MakeAWizard(void *data)
{
    static int posInc;
    World *world = (World *)data;

    Sol_Prefab_Wizard(world, (vec3s){0, posInc, 0});
    posInc++;
}