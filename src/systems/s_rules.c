/*
 * File: s_rules.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-22
 *
 */
#include "world.h"

static void EndPhase(World *world, SlRules *rules)
{
    if (rules->phase == RULESPHASE_END)
        return;

    rules->phase = RULESPHASE_END;
    rules->on_win(world, rules);
}

static void FFA(World *world, double dt, SlRules *rules)
{
    if (rules->as.ffa.winning_kills == 0)
        return;
    switch (rules->phase)
    {
    case RULESPHASE_ACTIVE: {
        SparseSet_ScScore *score_set = Sol_Comp_Set(world, ScScore);
        for (int i = 0; i < score_set->cnt; i++)
        {
            int id         = score_set->dense[i];
            ScScore *score = &score_set->data[i];
            if (score->kills >= rules->as.ffa.winning_kills)
            {
                rules->winner = id;
                EndPhase(world, rules);
                break;
            }
        }
    }
    break;
    case RULESPHASE_END: {
    }
    break;
    }
}

static void TDM(World *world, double dt, SlRules *rules)
{
    int team_kills[2]            = {0};
    SparseSet_ScScore *score_set = Sol_Comp_Set(world, ScScore);
    for (int i = 0; i < score_set->cnt; i++)
    {
        int id         = score_set->dense[i];
        ScScore *score = &score_set->data[i];
        ScTeam *team   = Sol_Comp_Get(world, id, ScTeam);
        for (int j = 0; j < 2; j++)
        {
            if (team->team == j)
                team_kills[j] += score->kills;
        }
    }
    for (int j = 0; j < 2; j++)
        if (team_kills[j] >= rules->as.tdm.winning_kills)
        {
            rules->winner = j;
            rules->phase  = RULESPHASE_END;
            break;
        }
}

static void CrystalPush(World *world, double dt, SlRules *rules)
{
}

void (*const rules_kind_update[RULESKIND_COUNT])(World *world, double dt, SlRules *rules) = {
    [RULESKIND_FFA]         = FFA,
    [RULESKIND_TDM]         = TDM,
    [RULESKIND_CRYSTALPUSH] = CrystalPush,
};

void Rules_Update(World *world, double dt)
{
    SlRules *rules = Sol_Comp_Get(world, 0, SlRules);
    if (rules_kind_update[rules->kind])
        rules_kind_update[rules->kind](world, dt, rules);
    else
        FFA(world, dt, rules);
}
