#include "sol_core.h"

#include "view.h"

// static void Sol_AiController_Debug(World *world, double dt, double time)
// {
//     int required = HAS_ACTIVE | HAS_AICONTROLLER;
//     for (int i = 0; i < world->activeCount; i++)
//     {
//         int id = world->activeEntities[i];
//         if (Sol_Vital_GetDead(world, id) || (world->masks[id] & required) != required ||
//             world->replications[id].auth == NETAUTH_REMOTE)
//             continue;
//         CompController   *controller   = &world->controllers[id];
//         CompAiController *aicontroller = &world->aicontrollers[id];
//         vec3s pos = Sol_Xform_GetDrawXform(world, id).pos;
//         pos.y += Sol_Physx_GetDims(world, id).y * 0.77f;
        
//     }
// }
