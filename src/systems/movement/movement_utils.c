#include "sol_core.h"

#include "movement_i.h"

void CrouchHeight(World *world, int id, float fdt)
{
    CompMovement *move          = &world->movements[id];
    float         currentHeight = Sol_Physx_GetHeight(world, id);
    float         difference    = fabs(currentHeight - move->targetHeight);
    if (difference < 0.001f)
        return;
    float newHeight = Sol_Math_Lerp(currentHeight, move->targetHeight, 5.0f * fdt);
    if (newHeight > currentHeight)
    {
        SolRayResult result =
            Sol_Raycast(world, (SolRay){.pos = Sol_Xform_GetPos(world, id), .dir = WORLD_UP, .dist = newHeight * 0.6f});
        if (result.hit)
            return;
    }
    Sol_Physx_SetHeight(world, id, newHeight);
    Sol_Model_SetOffsetY(world, id, newHeight * -0.5f);
}

void Knockback(World *world, int id, float fdt)
{
    CompMovement *move = &world->movements[id];
    if(move->knockDur > 0)
    {
        move->knockDur -= fdt;
        Sol_Physx_LerpVel(world, id, move->knockVel, 0.5f);
    }
    if (move->frictionMod != 1.0f)
    {
        float newFriction = Sol_Math_Lerp(move->frictionMod, 1.0f, 5.0f *fdt);
    }

}

vec3s GroundSlope(World *world, int id)
{
    vec3s normal     = Sol_Physx_GetGround(world, id);
    float dot        = glms_vec3_dot(WORLD_DOWN, normal);
    vec3s projection = glms_vec3_scale(normal, dot);
    vec3s slope      = glms_vec3_sub(WORLD_DOWN, projection);
    return glms_normalize(slope);
}

vec3s ProjectOntoGround(World *world, int id, vec3s wishdir)
{
    vec3s ground = Sol_Physx_GetGround(world, id);
    float dot    = glms_vec3_dot(wishdir, ground);
    return glms_vec3_sub(wishdir, glms_vec3_scale(ground, dot));
}