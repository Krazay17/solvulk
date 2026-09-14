/*
 * File: sl_spatial.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-12
 *
 */
#include "world.h"
#include "spatial_grid.h"

#include <omp.h>

void Sl_Spatial_Init(World *world)
{
    SlSpatial *spatial             = malloc(sizeof(SlSpatial));
    world->singles[SINGLE_SPATIAL] = spatial;
    solb_init(spatial->contacts, 4096);
    solb_init(spatial->tris_static, 4096);
    solb_init(spatial->build_ids, 64);
    solb_init(spatial->build_poss, 64);
    solb_init(spatial->build_extents, 64);

    int max_threads = omp_get_max_threads();
    solb_init(spatial->threadContacts, max_threads);
    solb_set_count(spatial->threadContacts, max_threads);
    solb_init(spatial->threadIds, max_threads);
    solb_set_count(spatial->threadIds, max_threads);

    for (int i = 0; i < max_threads; i++)
    {
        solb_init(spatial->threadContacts[i].contacts, 256);
        solb_init(spatial->threadIds[i].ids, 32);
    }

    {
        float cell_size       = 4.0f;
        spatial->grid_dynamic = malloc(sizeof(SpatialGrid));
        vec3s min             = {-512.0f, -512.0f, -32.0f};
        vec3s max             = {512.0f, 512.0f, 128.0f};
        SpatialGrid_Init(spatial->grid_dynamic, min, max, cell_size);
    }

    {
        float cell_size      = 4.0f;
        spatial->grid_static = malloc(sizeof(SpatialGrid));
        vec3s min            = {-512.0f, -512.0f, -32.0f};
        vec3s max            = {512.0f, 512.0f, 128.0f};
        SpatialGrid_Init(spatial->grid_static, min, max, cell_size);
    }
}