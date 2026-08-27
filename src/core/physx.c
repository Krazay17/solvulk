#include "physx.h"

// ShapeTriTest shape_tri_test[SHAPE3_CNT] = {
//     [SHAPE3_SPH] = Collide_Sphere_Tri,
//     [SHAPE3_CAP] = Collide_Capsule_Tri,
// };

// ShapePairTest shape_pair_test[SHAPE3_CNT][SHAPE3_CNT] = {
//     [SHAPE3_SPH][SHAPE3_SPH] = Collide_Sphere_Sphere,  [SHAPE3_CAP][SHAPE3_CAP] = Collide_Capsule_Capsule,
//     [SHAPE3_CAP][SHAPE3_SPH] = Collide_Capsule_Sphere, [SHAPE3_SPH][SHAPE3_CAP] = Collide_Sphere_Capsule,
//     [SHAPE3_MOD][SHAPE3_MOD] = Collide_Sphere_Sphere,  [SHAPE3_MOD][SHAPE3_SPH] = Collide_Sphere_Sphere,
//     [SHAPE3_BOX][SHAPE3_BOX] = Collide_Box_Box,        [SHAPE3_BOX][SHAPE3_SPH] = Collide_Sphere_Box,
//     [SHAPE3_SPH][SHAPE3_BOX] = Collide_Sphere_Box,
// };

SubstepData Substep_Get(float speed, float radius, float fdt)
{
    SubstepData substep_data;

    u8 substeps = (int)ceilf(speed * fdt / radius);
    if (substeps < 1)
        substeps = 1;
    if (substeps > 16)
        substeps = 16;

    substep_data.substeps = substeps;
    substep_data.sub_dt   = fdt / substeps;

    return substep_data;
}
