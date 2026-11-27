#ifndef ISLAND_PLAYER_COLLISION_H
#define ISLAND_PLAYER_COLLISION_H

#include "player_movement.h"

/* Height and land queries for the original Island of Ashes terrain. */
float island_ground_height(float x, float z);
int island_is_land(float x, float z);

/* Resolve movement against shoreline and simple building collision boxes. */
void island_resolve_player(PlayerMoveState *state,
                           float previous_x, float previous_z,
                           float radius);

#endif
