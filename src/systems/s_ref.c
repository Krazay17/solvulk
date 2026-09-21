#include "world.h"
#include "sol_core.h"

void Ref_Update(World *world, double dt)
{
    SparseSet_ScRef *set = Sol_Comp_Set(world, ScRef);
    int count            = set->cnt;
    for (int i = 0; i < count; i++)
    {
        int id           = set->dense[i];
        ScRef *ref       = &set->data[i];
        World *ref_world = Sol_GetWorldByIdx(ref->ent_world);
        int ref_id       = ref->ent_id;
        switch (ref->kind)
        {
        case REFKIND_HEALTHBAR:
            ScView2 *view2   = Sol_Comp_Get(world, id, ScView2);
            ScCombat *combat = Sol_Comp_Get(ref_world, ref_id, ScCombat);
            if (combat && view2)
            {
                snprintf(view2->views[5].text, sizeof(view2->views[5].text), "%.0f", combat->health);
            }
            break;
        }
    }
}