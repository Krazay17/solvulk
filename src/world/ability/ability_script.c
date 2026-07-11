#include "ability_i.h"
#include "sol_core.h"
#include "sol_math.h"
#include "world.h"
#include "audio.h"
#include "game/prefabs.h"
#include "model/s_model.h"
#include "physx/s_body.h"
#include "controller/s_controller.h"
#include "owner/s_owner.h"
#include "event/s_event.h"

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

static void Ability_ExecuteAction(World *world, int id, AbilityAction *a);

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
    fireballVolley->actions[0]    = (AbilityAction){
        .kind    = ACTIONKIND_PLAY_ANIM,
        .as.anim = (AnimDesc){.anim = ANIM_ABILITY4, .layerId = ANIM_LAYER_OVERRIDE, .playKind = ANIMPLAYKIND_ONESHOT}};
    fireballVolley->actions[1] = (AbilityAction){.kind           = ACTIONKIND_SPAWN_PROJECTILE,
                                                 .as.spawn.spawn = ABILITYSPAWN_FIREBALL,
                                                 .as.spawn.speed = 25.0f,
                                                 .time           = 0.75f};
    fireballVolley->actions[2] = (AbilityAction){.kind           = ACTIONKIND_SPAWN_PROJECTILE,
                                                 .as.spawn.spawn = ABILITYSPAWN_FIREBALL,
                                                 .as.spawn.speed = 30.0f,
                                                 .time           = 1.7f};
}

void Script_State_Update(World *world, int id, float dt)
{
    CompAbility   *ability = &world->abilities[id];
    AbilityData   *data    = &ability->stateData[ability->activeSlot];
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
        Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0, true);
}

void Script_State_Enter(World *world, int id)
{
}

void Script_State_Exit(World *world, int id)
{
    Sol_Model_PlayAnim(world, id, (AnimDesc){.layerId = ANIM_LAYER_OVERRIDE});
}

bool Script_State_CanExit(World *world, int id, u32 nextState)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &world->abilities[id].stateData[ability->activeSlot];

    return data->elapsed >= ability_scripts[ability->state].duration * 0.5f;
}

bool Script_State_CanEnter(World *world, int id, u32 lastState, u32 next, int slot)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &world->abilities[id].stateData[slot];

    double now     = solState.gameTime;
    double readyAt = data->lastEntered + ability_scripts[next].cooldown;

    return now > readyAt;
}

static void Ability_ExecuteAction(World *world, int id, AbilityAction *a)
{
    switch (a->kind)
    {
    case ACTIONKIND_SPAWN_PROJECTILE:
        int fireball = Sol_Prefab_Factory(world, 0, EKIND_FIREBALL,
                                          (EntDesc){.pos = Sol_Controller_GetShootPos(world, id, 0.2f), .scale = 0.5f});
        if (fireball > 0)
        {
            Sol_Physx_SetVel(world, fireball, vecSca(Sol_Controller_GetAimdir(world, id), a->as.spawn.speed));
            Sol_Owner_Add(world, fireball, id);
        }

        break;
    case ACTIONKIND_PLAY_ANIM:
        Sol_Model_PlayAnim(world, id, a->as.anim);

        break;
    case ACTIONKIND_PLAY_SOUND:
        Sol_Event_Add(world, (SolEvent){
                                 .kind            = EVENTKIND_SOUND,
                                 .as.sound.kind   = a->as.sound.sound,
                                 .as.sound.pos    = Sol_Controller_GetAimPos(world, id),
                                 .as.sound.volume = 1.0f,
                             });
        break;
    case ACTIONKIND_APPLY_BUFF:

        break;
    case ACTIONKIND_END:

        break;
    }
}