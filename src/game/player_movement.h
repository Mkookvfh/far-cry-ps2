#ifndef ISLAND_PLAYER_MOVEMENT_H
#define ISLAND_PLAYER_MOVEMENT_H

typedef struct {
    float x;
    float z;
    float stamina;
} PlayerMoveState;

/* Returns 1 while sprint is active, otherwise 0. */
int player_move_update(PlayerMoveState *state,
                      float input_x, float input_z,
                      float yaw_degrees, int sprint_requested,
                      float min_x, float max_x,
                      float min_z, float max_z);

#endif
