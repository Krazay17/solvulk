#include "s_ai.h"
#include "world.h"
#include "sol_math.h"

void Ai_Step(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_ScAi *set = Sol_Comp_Set(world, ScAi);
    for (int i = 0; i < set->cnt; i++)
    {
        int id      = set->dense[i];
        ScAi *ai    = &set->data[i];
        ScCmd *cmd  = Sol_Comp_Get(world, id, ScCmd);
        Xform xform = Xform_Get(world, id);
        if (!cmd)
            continue;

        cmd->aimdir  = Sol_Vec3_FromYawPitch(cmd->yaw, cmd->pitch);

        if (Sol_Comp_Has(world, id, ScTeam))
        {
            ScTeam *team = Sol_Comp_Get(world, id, ScTeam);
            ai->target   = Find_Target(world, id, ai, cmd, team->team);
            if (ai->target)
            {
                ai->last_target    = ai->target;
                ai->dropAggroTimer = 20.0f;
            }
        }

        if (ai->last_target)
        {
            ai->dirToTarget    = vecNorm(vecSub(Xform_Get(world, ai->last_target).pos, xform.pos));
            ai->dropAggroTimer = fmaxf(0.0f, ai->dropAggroTimer - fdt);
            if (ai->dropAggroTimer <= 0.0f)
                ai->last_target = 0;
        }

        AiStateFuncs funcs = Ai_Get_Funcs(ai->kind, ai->state);
        if (funcs.update)
            funcs.update(world, id, ai, fdt);
    }
}