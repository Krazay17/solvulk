#include "sol_core.h"

Sol_System_Interact_Ui(World *world, double dt, double time)
{
    int required = HAS_INTERACT;
    SolMouse mouse = SolInput_GetMouse();
    FOREACH_ENT(world, required, id)
    {
        
    }
}