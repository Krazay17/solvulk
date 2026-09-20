Inline optimized system structure

```c
#include "world.h"

typedef enum {
    BUFF_APPLY,
    BUFF_UPDATE,
    BUFF_REMOVE,
} BuffPhase;

// 1. Handlers remain static inline
static inline void Fire_OnApply(World *world, int id, Buff *buff) {}
static inline void Fire_OnRemove(World *world, int id, Buff *buff) {}

static inline void Fire_OnUpdate(World *world, int id, Buff *buff)
{
    if (buff->accum >= buff->rate)
    {
        buff->accum -= buff->rate;
        ScCombat *combat = Sol_Comp_Get(world, id, ScCombat);
        if (combat)
            Sol_Combat_Damage(world, id, combat, buff->power * 2.0f);
    }
}

// 2. Single dispatcher — all callbacks for a buff live in ONE block
static inline void Buff_Dispatch(World *world, int id, Buff *buff, BuffPhase phase)
{
    switch (buff->kind)
    {
        case BUFFKIND_FIRE:
            if (phase == BUFF_APPLY)  Fire_OnApply(world, id, buff);
            if (phase == BUFF_UPDATE) Fire_OnUpdate(world, id, buff);
            if (phase == BUFF_REMOVE) Fire_OnRemove(world, id, buff);
            break;

        case BUFFKIND_POISON:
            // Easy to add next buff in just ONE spot:
            // if (phase == BUFF_APPLY)  Poison_OnApply(world, id, buff);
            // if (phase == BUFF_UPDATE) Poison_OnUpdate(world, id, buff);
            // if (phase == BUFF_REMOVE) Poison_OnRemove(world, id, buff);
            break;

        default: break;
    }
}

// 3. Hot loop calls the single dispatcher with a compile-time constant phase
void Buff_Update(World *world)
{
    float fdt             = world->fdt;
    SparseSet_ScBuff *set = Sol_Comp_Set(world, ScBuff);
    int count             = set->cnt;

    for (int i = count; i-- > 0;)
    {
        int id        = set->dense[i];
        ScBuff *buffs = &set->data[i];
        int write     = 0;

        for (int b = 0; b < buffs->count; b++)
        {
            Buff *buff = &buffs->buffs[b];
            buff->elapsed += fdt;
            buff->accum += fdt;

            if (!buff->hasUpdated)
            {
                // Compiler inlines this call, sees phase == BUFF_APPLY, 
                // and strips out UPDATE and REMOVE branches completely.
                Buff_Dispatch(world, id, buff, BUFF_APPLY);
                buff->hasUpdated = 1;
            }

            Buff_Dispatch(world, id, buff, BUFF_UPDATE);

            if (buff->elapsed >= buff->duration)
            {
                Buff_Dispatch(world, id, buff, BUFF_REMOVE);
            }
            else
            {
                buffs->buffs[write++] = *buff;
            }
        }
        buffs->count = write;
        if (buffs->count <= 0)
            Sol_Comp_Rem(world, id, ScBuff);
    }
}