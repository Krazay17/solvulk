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
    ThreadIdBuffer *threadIds;
    SolTri *tris_static;
    uint32_t *build_ids;
    vec3s *build_poss;
    vec3s *build_extents;
} SlSpatial;
