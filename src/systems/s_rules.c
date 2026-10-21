#include "world.h"

static void FFA(World *world, double dt, SlRules *rules)
{
    SparseSet_ScScore *score_set = Sol_Comp_Set(world, ScScore);
    for (int i = 0; i < score_set->cnt; i++)
    {
        int id         = score_set->dense[i];
        ScScore *score = &score_set->data[i];
        if (score->kills >= rules->as.ffa.winning_kills)
        {
            rules->winner = id;
            rules->phase  = RULESPHASE_END;
        }
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

static void Draw_FFA(World *world, double dt, SlRules *rules)
{
}

void (*const rules_kind_update[RULESKIND_COUNT])(World *world, double dt, SlRules *rules) = {
    [RULESKIND_FFA]         = FFA,
    [RULESKIND_TDM]         = TDM,
    [RULESKIND_CRYSTALPUSH] = CrystalPush,
};
void (*const rules_kind_draw[RULESKIND_COUNT])(World *world, double dt, SlRules *rules) = {
    [RULESKIND_FFA] = Draw_FFA,
    // [RULESKIND_TDM]         = TDM,
    // [RULESKIND_CRYSTALPUSH] = CrystalPush,
};

void Rules_Update(World *world, double dt)
{
    SlRules *rules = Sol_Comp_Get(world, 0, SlRules);
    if (rules_kind_update[rules->kind])
        rules_kind_update[rules->kind](world, dt, rules);
    else
        FFA(world, dt, rules);
}

void Rules_Draw(World *world, double dt)
{
    SlRules *rules = Sol_Comp_Get(world, 0, SlRules);
    if (rules_kind_draw[rules->kind])
        rules_kind_draw[rules->kind](world, dt, rules);
    else
        Draw_FFA(world, dt, rules);
}
