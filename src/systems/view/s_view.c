#include "sol_core.h"

void Sol_View_Init(World *world)
{
    Sol_View_Buff(world);
    Sol_View_Ability(world);
    Sol_View_Healthbar(world);
    Sol_View_Fx(world);
}