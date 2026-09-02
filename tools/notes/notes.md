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

Think of a __m128 register as a 4-slot array ([slot 0, slot 1, slot 2, slot 3]) inside a dedicated CPU register.Here is a step-by-step breakdown:Step 1: Loading Data into Vector RegistersC__m128 vpos = _mm_setr_ps(pos.x, pos.y, pos.z, 0.0f);
What it does: Creates a vector variable vpos containing [pos.x, pos.y, pos.z, 0.0f].Breakdown: _mm_setr_ps stands for Set Reverse Packed Single-precision. It packs 4 floats into one register in normal left-to-right order.C__m128 vmin = _mm_loadu_ps(&grid->min.x);
__m128 vinv = _mm_loadu_ps(&grid->invCellSizeVec.x);
What it does: Reads 16 contiguous bytes (4 floats) directly from memory into vmin and vinv.Breakdown: _mm_loadu_ps stands for Load Unaligned Packed Single-precision. It grabs min.x, min.y, min.z, and min.w all in one single RAM-to-CPU transfer instruction.Step 2: The Core Math (In Parallel)C__m128 vscaled = _mm_mul_ps(_mm_sub_ps(vpos, vmin), vinv);
What it does: Calculates (vpos - vmin) * vinv for all axes simultaneously.Breakdown:_mm_sub_ps (Subtract Packed Single-precision) computes:[pos.x - min.x, pos.y - min.y, pos.z - min.z, 0.0 - 0.0]_mm_mul_ps (Multiply Packed Single-precision) takes that result and multiplies it by vinv:[dx * inv, dy * inv, dz * inv, 0.0]Step 3: Rounding Down and Converting to IntegerC#if defined(__SSE4_1__)
    __m128  vfloored = _mm_floor_ps(vscaled);
    __m128i vi       = _mm_cvttps_epi32(vfloored);
#else
    __m128i vi       = _mm_cvttps_epi32(_mm_sub_ps(vscaled, _mm_set1_ps(0.5f)));
#endif
What it does: Rounds the floats down (floor) and casts them from float to int.Breakdown:_mm_floor_ps: Rounds floats down to the nearest lower integer in float format (e.g., 3.7 $\rightarrow$ 3.0, -1.2 $\rightarrow$ -2.0). This requires SSE4.1 support (standard on CPUs from 2008+)._mm_cvttps_epi32: Stands for Convert Truncated Packed Single-precision to Extended Packed 32-bit Integers (__m128i). It converts floats to integer representation by stripping decimals.#else block: If compiling for older CPUs without SSE4.1, subtracting 0.5f before truncating simulates floorf() behavior for positive and negative floating-point numbers using base SSE2.Step 4: Extracting the OutputCint tmp[4];
_mm_storeu_si128((__m128i*)tmp, vi);

return (ivec3s){ tmp[0], tmp[1], tmp[2] };
What it does: Writes the 4 integers from the vector register vi into a local integer array tmp so you can return your ivec3s struct.Breakdown: _mm_storeu_si128 takes a 128-bit integer vector and dumps its 16 bytes straight into memory at &tmp[0].Why this is worth the weird syntaxInstead of making the CPU bounce between scalar registers and float units 9 different times, this entire function compiles down to 5 to 7 raw assembly instructions total, running with zero conditional branches.

// Insertion sort (Fastest for small hit buffers like 8-64 elements)
for (int i = 1; i < hits; i++)
{
    SolRayResult key = result[i];
    int j = i - 1;
    while (j >= 0 && result[j].dist > key.dist)
    {
        result[j + 1] = result[j];
        j--;
    }
    result[j + 1] = key;
}