#include "world.h"
#include "spatial_grid.h"
#include <omp.h>

void SlEmitter_Init(World *world, SlEmitter *self)
{
    solb_init(self->emitters, 128);
    solb_init(self->particles, 512);
}

void SlEmitter_Deinit(SlEmitter *self)
{
    solb_free(self->emitters);
    solb_free(self->particles);
}

void SlHitgen_Init(World *world, SlHitgen *self)
{
    memset(self->matrix, 0, sizeof(self->matrix));
    self->global = 1;
}

void SlHitgen_Deinit(SlHitgen *self)
{
}

void SlSpatial_Init(World *world, SlSpatial *self)
{
    solb_init(self->contacts, 4096);
    solb_init(self->tris_static, 4096);
    solb_init(self->build_ids, 64);
    solb_init(self->build_mins, 64);
    solb_init(self->build_maxs, 64);

    int max_threads = omp_get_max_threads();
    solb_init(self->threadContacts, max_threads);
    solb_set_count(self->threadContacts, max_threads);
    solb_init(self->threadIds, max_threads);
    solb_set_count(self->threadIds, max_threads);

    for (int i = 0; i < max_threads; i++)
    {
        solb_init(self->threadContacts[i].contacts, 256);
        solb_init(self->threadIds[i].ids, 32);
    }

    {
        float cell_size       = 4.0f;
        self->grid_dynamic = malloc(sizeof(SpatialGrid));
        vec3s min             = {-512.0f, -512.0f, -32.0f};
        vec3s max             = {512.0f, 512.0f, 128.0f};
        SpatialGrid_Init(self->grid_dynamic, min, max, cell_size);
    }

    {
        float cell_size      = 4.0f;
        self->grid_static = malloc(sizeof(SpatialGrid));
        vec3s min            = {-512.0f, -512.0f, -32.0f};
        vec3s max            = {512.0f, 512.0f, 128.0f};
        SpatialGrid_Init(self->grid_static, min, max, cell_size);
    }
}

void SlSpatial_Deinit(SlSpatial *self)
{
    solb_free(self->grid_dynamic);
    solb_free(self->grid_static);
    solb_free(self->contacts);
    solb_free(self->threadIds);
    solb_free(self->tris_static);
    solb_free(self->build_ids);
    solb_free(self->build_mins);
    solb_free(self->build_maxs);
}

u32 Sol_Hitgen_Start(World *world, int id)
{
    SlHitgen *single = Sol_Comp_Get(world, 0, SlHitgen);

    single->global++;
    if (single->global == 0)
    {
        memset(single->matrix, 0, sizeof(single->matrix));
        single->global = 1;
    }
    return single->global;
}

bool Sol_Hitgen_Try(World *world, int id, int target, u32 sessionGen)
{
    SlHitgen *single = Sol_Comp_Get(world, 0, SlHitgen);

    if (single->matrix[id][target] == sessionGen)
        return false;

    single->matrix[id][target] = sessionGen;

    return true;
}
