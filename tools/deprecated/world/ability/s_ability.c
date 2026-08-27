#include "ability/si_ability.h"
#include "sol_core.h"
#include "world.h"
#include "sol_math.h"

#include "xform/s_xform.h"
#include "model/s_model.h"
#include "physx/s_body.h"
#include "combat/s_combat.h"
#include "controller/s_controller.h"
#include "buff/s_buff.h"
#include "replication/s_replication.h"
#include "combat/s_combat.h"
#include "render/render.h"
#include "s_ability.h"

typedef struct
{
    int          cnt, cap;
    uint32_t    *sparse, *dense;
    CompAbility *abilities;
} WorldAbilities;

static void Ability_Step(World *world, double dt, double time)
{
    WorldAbilities *wc = world->dense_components[WORLD_SYS_ABILITY];
    for (int i = 0; i < wc->cnt; i++)
    {
        int          id      = wc->dense[i];
        CompAbility *ability = &wc->abilities[i];

        if (Sol_Combat_GetDead(world, id))
        {
            Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0, true);
            continue;
        }
        if (Sol_Buff_HasBuff(world, id, BUFFKIND_STUN))
        {
            Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0, true);
            continue;
        }

        if (ability_state_func[ability->state].update)
            ability_state_func[ability->state].update(world, id, dt);
    }
}

static void Ability_Draw(World *world, double dt, double time)
{
    WorldAbilities *wc = world->dense_components[WORLD_SYS_ABILITY];
    for (int i = 0; i < wc->cnt; i++)
    {
        int          id      = wc->dense[i];
        CompAbility *ability = &wc->abilities[i];
        if (ability_state_func[ability->state].draw)
            ability_state_func[ability->state].draw(world, id, dt, time);
    }
}

void Sol_Ability_Init(World *world)
{
    WorldAbilities *wc                         = malloc(sizeof(WorldAbilities));
    world->dense_components[WORLD_SYS_ABILITY] = wc;
    wc->cap                                    = 64;
    wc->cnt                                    = 0;
    wc->sparse                                 = malloc(MAX_ENTS * sizeof(uint32_t));
    wc->dense                                  = malloc(wc->cap * sizeof(uint32_t));
    wc->abilities                              = malloc(wc->cap * sizeof(CompAbility));
    memset(wc->sparse, INVALID_INDEX, MAX_ENTS * sizeof(uint32_t));

    WAddStep(world) = Ability_Step;
    WAdd3d(world)   = Ability_Draw;

    Ability_Scripts_Init();
}

CompAbility *Sol_Ability_Add(World *world, int id, AbilityDesc desc)
{
    WorldAbilities *wc = world->dense_components[WORLD_SYS_ABILITY];
    if (wc->sparse[id] != INVALID_INDEX)
        return &wc->abilities[wc->sparse[id]];
    if (wc->cnt >= wc->cap)
    {
        wc->cap *= 2;
        wc->dense     = realloc(wc->dense, wc->cap * sizeof(uint32_t));
        wc->abilities = realloc(wc->abilities, wc->cap * sizeof(CompAbility));
    }

    u32 denseIdx        = wc->cnt++;
    wc->sparse[id]      = denseIdx;
    wc->dense[denseIdx] = id;

    CompAbility *ability = &wc->abilities[denseIdx];
    memcpy(ability->action_map, desc.ability_map, sizeof(int) * ABILITY_SLOTS);
    ability->activeSlot = -1;
    ability->state      = 0;
    for (int i = 0; i < ABILITY_SLOTS; i++)
    {
        ability->stateData[i].lastEntered = -FLT_MAX;
        ability->stateData[i].lastExited  = -FLT_MAX;
        ability->stateData[i].stage       = 0;
        ability->stateData[i].elapsed     = 0.0f;
    }

    return ability;
}

CompAbility *Sol_Ability_Get(World *world, int id)
{
    WorldAbilities *wc = world->dense_components[WORLD_SYS_ABILITY];
    if (!wc || wc->sparse[id] == INVALID_INDEX)
        return NULL;
    return &wc->abilities[wc->sparse[id]];
}

AbilityStateData *Sol_Ability_GetActiveSlotData(World *world, int id)
{
    CompAbility *ability = Sol_Ability_Get(world, id);
    return &ability->stateData[ability->activeSlot];
}

bool Sol_Ability_Has(World *world, int id)
{
    WorldAbilities *wc = world->dense_components[WORLD_SYS_ABILITY];
    return wc->sparse[id] != INVALID_INDEX;
}

bool Sol_Ability_SetState(World *world, int id, AbilityState nextState, int slot, bool force)
{
    if (nextState > ABILITY_STATE_COUNT)
        return false;
    CompAbility     *ability  = Sol_Ability_Get(world, id);
    const StateFunc *prevfunc = &ability_state_func[ability->state];
    const StateFunc *nextfunc = &ability_state_func[nextState];

    if (!force)
    {
        if (!prevfunc->canExit || !prevfunc->canExit(world, id, nextState))
            return false;
        if (!nextfunc->canEnter || !nextfunc->canEnter(world, id, ability->state, nextState, slot))
            return false;
    }

    prevfunc->exit(world, id);
    ability->state                       = nextState;
    ability->activeSlot                  = slot;
    ability->stateData[slot].elapsed     = 0;
    ability->stateData[slot].accum       = 0;
    ability->stateData[slot].lastEntered = solState.gameTime;
    nextfunc->enter(world, id);

    return true;
}