#include "s_rules.h"
#include "world.h"
#include "sol_math.h"
#include "prefabs/prefabs.h"

static inline void Victory(World *world, SlRules *rules)
{
    if (rules->phase == RULESPHASE_END)
        return;

    rules->phase = RULESPHASE_END;

    if (rules->winner_team == 0)
        rules->red_wins++;
    else if (rules->winner_team == 1)
        rules->green_wins++;

    if (rules->on_win)
        rules->on_win(world, rules);
}

static inline void Active(World *world, double dt, SlRules *rules)
{
    int crystal_a = rules->as.crystal_push.crystal_a;
    int crystal_b = rules->as.crystal_push.crystal_b;
    forc(world, crystal_a, ScCombat) if (c->is_dead)
    {
        rules->winner_team = 1;
        Victory(world, rules);
        return;
    }
    forc(world, crystal_b, ScCombat) if (c->is_dead)
    {
        rules->winner_team = 0;
        Victory(world, rules);
        return;
    }
    float wave_rate   = rules->as.crystal_push.wave_rate;
    float *wave_accum = &rules->as.crystal_push._wave_accum;
    *wave_accum += dt;
    while (wave_rate > 0 && *wave_accum >= wave_rate)
    {
        *wave_accum -= wave_rate;
        rules->as.crystal_push._pending_minion_count += rules->as.crystal_push.wave_minion_count;
        rules->as.crystal_push._minion_accum = rules->as.crystal_push.minion_rate;
    }
    if (rules->as.crystal_push._pending_minion_count > 0)
    {
        float minion_rate   = rules->as.crystal_push.minion_rate;
        float *minion_accum = &rules->as.crystal_push._minion_accum;
        *minion_accum += dt;
        while (minion_rate > 0 && *minion_accum >= minion_rate)
        {
            *minion_accum -= minion_rate;
            rules->as.crystal_push._pending_minion_count--;
            Sol_Prefab_Minion(world, (vec3s){0, 2, -85.0f}, glms_euler_xyz_quat((vec3s){0.0f, glm_rad(180.0f), 0.0f}),
                              0);
            Sol_Prefab_Minion(world, (vec3s){0, 2, 85.0f}, GLMS_QUAT_IDENTITY, 1);
        }
    }
}

static inline void Respawn(World *world, SlRules *rules)
{
    forc(world, 0, SlEvent)
    {
        int count = solb_count(c->events);
        for (int i = 0; i < count; i++)
        {
            SolEvent event = c->events[i];
            if (event.kind != EVENTKIND_RESPAWN)
                continue;
            int id = event.dealer;
            forc(world, id, ScTeam)
            {
                if (c->team >= 0)
                    world->xform.pos[id] =
                        c->team == 0 ? rules->as.crystal_push.spawn_a_player : rules->as.crystal_push.spawn_b_player;
            }
        }
    }
}

void CrystalPush(World *world, double dt, SlRules *rules)
{
    Respawn(world, rules);
    switch (rules->phase)
    {
    case RULESPHASE_ACTIVE:
        Active(world, dt, rules);
        break;
    }
}

static vec3s FindBossPos(World *world, int idx)
{
    vec3s pos = {0};
    for_set(world, ScStage)
    {
        int id = set->dense[i];
        forc(world, id, ScModel)
        {
            char buffer[32] = "PREFAB_Boss1";
            snprintf(buffer, sizeof(buffer), "PREFAB_Boss%d", idx);
            ModelPrefab *prefab = Sol_Model_GetPrefab(c, buffer);
            if (prefab)
            {
                pos = prefab->pos;
                return pos;
            }
        }
    }
    return pos;
}

void CrystalPush_Start(World *world)
{
    forc(world, 0, SlRules)
    {
        c->kind  = RULESKIND_CRYSTALPUSH;
        c->phase = RULESPHASE_ACTIVE;

        c->as.crystal_push.wave_rate         = 20.0f;
        c->as.crystal_push._wave_accum       = c->as.crystal_push.wave_rate;
        c->as.crystal_push.wave_minion_count = 6;
        c->as.crystal_push.minion_rate       = 1.0f;

        c->as.crystal_push.spawn_a_player = (vec3s){0, 3, -80.0f};
        c->as.crystal_push.spawn_b_player = (vec3s){0, 3, 80.0f};

        c->as.crystal_push.bosses[0] = Sol_Prefab_Bozzer(world, FindBossPos(world, 0));
        c->as.crystal_push.bosses[1] = Sol_Prefab_Bozzer(world, FindBossPos(world, 1));
        c->as.crystal_push.bosses[2] = Sol_Prefab_Bozzer(world, FindBossPos(world, 2));
        c->as.crystal_push.bosses[3] = Sol_Prefab_Bozzer(world, FindBossPos(world, 3));

        c->as.crystal_push.crystal_a = Sol_Prefab_Crystal(world, (vec3s){0, 3, -90.0f}, 0);
        c->as.crystal_push.crystal_b = Sol_Prefab_Crystal(world, (vec3s){0, 3, 90.0f}, 1);
    }
}