My Notes!

look at qu3e

I want to remove and compact my tris from my spatial grid when destroying a physx entity.


void Sol_Physx_Remove(World *world, int id)
{
    PhysxGroup *group = &world->spatial->staticGroup;
    PhysxEnts *ents = &world->spatial->staticGroup.ents[id];
    for (int i = ents->triIndexStart;  i< ents->triIndexCount;i++)
    {
        memset(&group->tris[i], 0, sizeof(SolTri));
    }
    group->triCount -= ents->triIndexCount;
}

LastState: 3, CurrentState: 4
LastState: 1, CurrentState: 0
LastState: 4, CurrentState: 0
LastState: 0, CurrentState: 3