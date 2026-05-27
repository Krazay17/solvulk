#include "sol_core.h"

#include "ability_i.h"

typedef enum
{
    ACTIONKIND_SPAWN_PROJECTILE,
    ACTIONKIND_PLAY_ANIM,
    ACTIONKIND_PLAY_SOUND,
    ACTIONKIND_APPLY_BUFF,
    ACTIONKIND_END,
} ActionKind;

typedef struct AbilityAction
{
    float time;

    ActionKind kind;
    union {
        AnimDesc anim;
        struct
        {
            AbilitySpawnId spawn;
            float          speed;
        } spawn;
        struct
        {
            SolAudioId sound;
        } sound;
    } as;
} AbilityAction;

typedef struct
{
    AbilityAction actions[16];
    int           actionCount;
    float         duration;
    float         cooldown;
} AbilityScript;

AbilityScript ability_scripts[ABILITY_STATE_COUNT];

void Ability_Scripts_Init()
{
    AbilityScript *fireballVolley = &ability_scripts[ABILITY_STATE_FIREBALLVOLLEY];
    fireballVolley->actionCount   = 3;
    fireballVolley->cooldown      = 6.0f;
    fireballVolley->duration      = 2.1f;
    fireballVolley->actions[0] =
        (AbilityAction){.kind    = ACTIONKIND_PLAY_ANIM,
                        .as.anim = (AnimDesc){.anim = ANIM_ABILITY4, .layerId = ANIM_LAYER_OVERRIDE, .force = true}};
    fireballVolley->actions[1] = (AbilityAction){.kind           = ACTIONKIND_SPAWN_PROJECTILE,
                                                 .as.spawn.spawn = ABILITYSPAWN_FIREBALL,
                                                 .as.spawn.speed = 25.0f,
                                                 .time           = 0.75f};
    fireballVolley->actions[2] = (AbilityAction){.kind           = ACTIONKIND_SPAWN_PROJECTILE,
                                                 .as.spawn.spawn = ABILITYSPAWN_FIREBALL,
                                                 .as.spawn.speed = 30.0f,
                                                 .time           = 1.7f};
}

void Ability_ExecuteAction(World *world, int id, AbilityAction *a);

void Script_State_Update(World *world, int id, float dt)
{
    CompAbility   *ability = &world->abilities[id];
    AbilityData   *data    = &ability->stateData[ability->state];
    AbilityScript *script  = &ability_scripts[ability->state];

    float oldElapsed = data->elapsed;
    data->elapsed += dt;

    for (int i = 0; i < script->actionCount; i++)
    {
        AbilityAction *a = &script->actions[i];
        if (a->time >= oldElapsed && a->time <= data->elapsed)
            Ability_ExecuteAction(world, id, a);
    }

    if (data->elapsed >= script->duration)
        Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE);
}

void Script_State_Enter(World *world, int id)
{
}

void Script_State_Exit(World *world, int id)
{
    Sol_Model_StopAnim(world, id, ANIM_LAYER_OVERRIDE, 0);
}

bool Script_State_CanExit(World *world, int id, u32 nextState)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &world->abilities[id].stateData[ability->state];

    return data->elapsed >= ability_scripts[ability->state].duration * 0.6f;
}

bool Script_State_CanEnter(World *world, int id, u32 lastState, u32 next)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &world->abilities[id].stateData[next];

    double now     = Sol_GetGameTime();
    double readyAt = data->lastEntered + ability_scripts[next].cooldown;

    return now > readyAt;
}

void Ability_ExecuteAction(World *world, int id, AbilityAction *a)
{
    switch (a->kind)
    {
    case ACTIONKIND_SPAWN_PROJECTILE:
        u32 fireball = Sol_Prefab_Factory(world, 0, PREFABKIND_FIREBALL, (PrefabDesc){.pos = Sol_Controller_GetShootPos(world, id, 0.2f), .scale = 0.5f});
        Sol_Physx_SetVel(world, fireball, vecSca(Sol_Controller_GetAimdir(world, id), a->as.spawn.speed));
        Sol_Owner_SetOwner(world, fireball, id);

        break;
    case ACTIONKIND_PLAY_ANIM:
        Sol_Model_PlayAnim(world, id, a->as.anim);

        break;
    case ACTIONKIND_PLAY_SOUND:
        Sol_Audio_PlayAt(a->as.sound.sound, Sol_Controller_GetAimPos(world, id), 0);
        break;
    case ACTIONKIND_APPLY_BUFF:

        break;
    case ACTIONKIND_END:

        break;
    }
}