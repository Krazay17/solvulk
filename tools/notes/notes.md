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

/*
Wizard anims:
Anim: 0 idle
Anim: 1 fwd
Anim: 2 attack1

Dude anims:
Model: Scene, Anim: 0 idle
Model: Scene, Anim: 1 WalkFwd
Model: Scene, Anim: 2 WalkLeft
Model: Scene, Anim: 3 WalkBwd
Model: Scene, Anim: 4 WalkRight
Model: Scene, Anim: 5 fall
Model: Scene, Anim: 6 jump
Model: Scene, Anim: 7 frontFlip
Model: Scene, Anim: 8 dash
Model: Scene, Anim: 9 dashLeft
Model: Scene, Anim: 10 dashBwd
Model: Scene, Anim: 11 dashRight
Model: Scene, Anim: 12 fallLeft
Model: Scene, Anim: 13 fallBwd
Model: Scene, Anim: 14 fallRight
Model: Scene, Anim: 15 attackRight
Model: Scene, Anim: 16 attackLeft
Model: Scene, Anim: 17 attackSpell
Model: Scene, Anim: 18 blade
Model: Scene, Anim: 19 bladeAir
Model: Scene, Anim: 20 runStopLeft
Model: Scene, Anim: 21 runStopFwd
Model: Scene, Anim: 22 runStopRight
Model: Scene, Anim: 23 knockback
Model: Scene, Anim: 24 spinSlash
Model: Scene, Anim: 25 LeftCharge
Model: Scene, Anim: 26 CrouchWalkFwd
Model: Scene, Anim: 27 SlideFwd
Model: Scene, Anim: 28 Stunned
Model: Scene, Anim: 29 LeftChannel
Model: Scene, Anim: 30 RightChannel
Model: Scene, Anim: 31 WallJumpLeft
Model: Scene, Anim: 32 WallJumpRight
Model: Scene, Anim: 33 Mantle
Model: Scene, Anim: 34 WallClimb
Model: Scene, Anim: 35 WallrunRight
Model: Scene, Anim: 36 WallrunLeft
Model: Scene, Anim: 37 MantleRoll
Model: Scene, Anim: 38 MantleRollRev
Model: Scene, Anim: 39 Jump2
*/
