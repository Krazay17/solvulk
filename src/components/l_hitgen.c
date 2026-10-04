#include "world.h"

typedef struct
{
    u32 session_gen;
    u32 *hit_targets; // solb stretchy buffer
} HitgenSession;

struct HitgenRow
{
    HitgenSession sessions[MAX_HITGEN_SESSIONS];
    u32 session_count;  // Active tracked slots (up to MAX_HITGEN_SESSIONS)
    u32 session_cursor; // FIFO replacement cursor when slots are full
};

void SlHitgen_Init(World *world, SlHitgen *self)
{
    memset(self->rows, 0, sizeof(self->rows));
    self->global = 1;
}

void SlHitgen_Deinit(SlHitgen *self)
{
    if (!self) return;

    for (int i = 0; i < MAX_ENTS; i++)
    {
        HitgenRow *row = self->rows[i];
        if (row)
        {
            for (u32 s = 0; s < MAX_HITGEN_SESSIONS; s++)
            {
                solb_free(row->sessions[s].hit_targets);
            }
            free(row);
            self->rows[i] = NULL;
        }
    }
}

u32 Sol_Hitgen_Start(World *world)
{
    SlHitgen *single = Sol_Comp_Get(world, 0, SlHitgen);

    single->global++;
    if (single->global == 0)
    {
        // On uint32 overflow, clean existing allocations to prevent dangling IDs
        SlHitgen_Deinit(single);
        single->global = 1;
    }
    return single->global;
}

bool Sol_Hitgen_Try(World *world, int id, int target, u32 sessionGen)
{
    if (id < 0 || id >= MAX_ENTS) return false;

    SlHitgen *single = Sol_Comp_Get(world, 0, SlHitgen);

    // 1. Lazy allocation for the attacker's HitgenRow
    HitgenRow *row = single->rows[id];
    if (!row)
    {
        row = (HitgenRow *)calloc(1, sizeof(HitgenRow));
        single->rows[id] = row;
    }

    // 2. Search active slots for matching sessionGen
    HitgenSession *session = NULL;
    for (u32 i = 0; i < row->session_count; i++)
    {
        if (row->sessions[i].session_gen == sessionGen)
        {
            session = &row->sessions[i];
            break;
        }
    }

    // 3. Existing session: Check for target collision
    if (session)
    {
        u32 count = solb_count(session->hit_targets);
        for (u32 i = 0; i < count; i++)
        {
            if (session->hit_targets[i] == (u32)target)
                return false; // Already hit this target during this session
        }

        solb_push(session->hit_targets, (u32)target);
        return true;
    }

    // 4. New session slot selection (append or FIFO eviction)
    u32 slot_idx;
    if (row->session_count < MAX_HITGEN_SESSIONS)
    {
        slot_idx = row->session_count++;
    }
    else
    {
        slot_idx = row->session_cursor;
        row->session_cursor = (row->session_cursor + 1) % MAX_HITGEN_SESSIONS;
    }

    session = &row->sessions[slot_idx];
    session->session_gen = sessionGen;

    // Reuse existing solb capacity by zeroing element count (0 heap allocations after warmup)
    solb_set_count(session->hit_targets, 0);
    solb_push(session->hit_targets, (u32)target);

    return true;
}

bool Sol_Hitgen_Has(World *world, int id, int target, u32 sessionGen)
{
    if (id < 0 || id >= MAX_ENTS) return false;

    SlHitgen *single = Sol_Comp_Get(world, 0, SlHitgen);
    HitgenRow *row = single->rows[id];
    if (!row) return false;

    for (u32 i = 0; i < row->session_count; i++)
    {
        if (row->sessions[i].session_gen == sessionGen)
        {
            HitgenSession *session = &row->sessions[i];
            for (u32 j = 0; j < solb_count(session->hit_targets); j++)
            {
                if (session->hit_targets[j] == (u32)target)
                    return true;
            }
            return false;
        }
    }
    return false;
}