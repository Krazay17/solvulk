#include "sol_core.h"

void Sol_Combat_Init(World *world)
{
    world->stepSystems[world->stepCount++] = Sol_Combat_Step;
}

void Sol_Combat_Step(World *world, double dt, double time)
{

}