#include "world.h"

void Rules_FFA(World *world, double dt, SlRules *rules)
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

void (*const rules_kind_update[RULESKIND_COUNT])(World *world, double dt, SlRules *rules) = {
    [RULESKIND_FFA] = Rules_FFA,
};

void Rules_Update(World *world, double dt)
{
    SlRules *rules = Sol_Comp_Get(world, 0, SlRules);
    if (rules_kind_update[rules->kind])
        rules_kind_update[rules->kind](world, dt, rules);
    else
        Rules_FFA(world, dt, rules);
}

void Rules_Draw(World *world, double dt)
{
}