/*
 * File: singles.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-13
 *
 */
#include "sol/types.h"
#include "spatial_grid.h"

typedef struct SlSpatial
{
    SpatialGrid *grid_dynamic;
    SpatialGrid *grid_static;
    SolContact *contacts;
    ThreadContactBuffer *threadContacts;
    IdBuffer *threadIds;
    SolTri *tris_static;
    u32 *build_ids;
    vec3s *build_mins;
    vec3s *build_maxs;
} SlSpatial;

typedef struct SlHitgen
{
    u32 global;
    u32 ent_gen[MAX_ENTS];
    u32 matrix[MAX_ENTS][256];
} SlHitgen;

typedef struct SlEmitter
{
    Emitter *emitters;
    Particle *particles;
} SlEmitter;

u32 Sol_Hitgen_Start(World *world, int id);
bool Sol_Hitgen_Try(World *world, int id, int target, u32 sessionGen);