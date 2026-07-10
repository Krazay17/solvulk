#include "s_combat.h"
#include "sol_math.h"
#include "sol_core.h"
#include "world.h"
#include "font.h"
#include "render/render.h"

#define BASE_TTL 2.0f
#define MAX_DAMAGE_FONT_SIZE 65.0f

void Sol_Dmgnumbers_Spawn(World *world, int id, int amnt, vec3s pos)
{
    Sol_Realloc((void **)&world->dmgNumbers->dmgNumber, world->dmgNumbers->count, &world->dmgNumbers->cap, sizeof(Dmgnumber));
    u32        idx       = world->dmgNumbers->count++;
    Dmgnumber *dmgNumber = &world->dmgNumbers->dmgNumber[idx];
    dmgNumber->color     = (vec4s){1.0f, 1.0f, 0.0f, 1.0f};
    dmgNumber->pos       = pos;
    dmgNumber->amnt      = amnt;
    dmgNumber->ttl       = BASE_TTL;
}

void Dmgnumbers_Step(World *world, double dt, double time)
{
    Dmgnumbers *dmgNumbers = world->dmgNumbers;
    int write = 0;
    for (int i = 0; i < dmgNumbers->count; i++)
    {
        Dmgnumber *dmgNumber = &dmgNumbers->dmgNumber[i];
        dmgNumber->ttl -= dt;
        if (dmgNumber->ttl <= 0)
            continue;
        dmgNumber->pos.y -= dt;

        dmgNumbers->dmgNumber[write++] = *dmgNumber;
    }
    dmgNumbers->count = write;
}

void Dmgnumbers_Draw(World *world, double dt, double time)
{
    Dmgnumbers *dmgNumbers = world->dmgNumbers;
    for (int i = 0; i < dmgNumbers->count; i++)
    {
        Dmgnumber *dmgNumber = &dmgNumbers->dmgNumber[i];
        char       buffer[8];
        sprintf(buffer, "%d", dmgNumber->amnt);
        float size = Sol_Math_Lerp(0.1f, 0.8f, (float)dmgNumber->amnt / MAX_DAMAGE_FONT_SIZE);
        Sol_Render_DrawText3D((Text3DDesc){.pos       = dmgNumber->pos,
                                           .color     = dmgNumber->color,
                                           .billboard = true,
                                           .inFront   = true,
                                           .font      = SOL_FONT_ICE,
                                           .size      = size,
                                           .text      = buffer});
    }
}
