#pragma once

void Respawn(World *world, int id, CompVital *vital);
void Die(World *world, int id, CompVital *vital);
void Damage(World *world, int id, CompVital *vital, SolHit hit);