#include "world.h"

void Sl_Emitter_Init(World *world)
{
    SlEmitter *single = malloc(sizeof(SlEmitter));
    world->singles[SINGLE_EMITTER] = single;

    solb_init(single->emitters, 128);
    solb_init(single->particles, 512);
}