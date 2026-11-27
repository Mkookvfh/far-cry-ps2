#include "player_movement.h"
#include <math.h>

int player_move_update(PlayerMoveState *state,
                       float input_x, float input_z,
                       float yaw_degrees, int sprint_requested,
                       float min_x, float max_x,
                       float min_z, float max_z)
{
    float yaw_radians;
    float world_dx;
    float world_dz;
    float move_speed;
    int moving;
    int sprinting;

    if (state == 0) return 0;

    /* Prevent faster diagonal movement, matching the prototype controls. */
    if (input_x != 0.0f && input_z != 0.0f) {
        input_x *= 0.7071f;
        input_z *= 0.7071f;
    }

    yaw_radians = yaw_degrees * 0.0174532925f;
    world_dx = input_x * cosf(yaw_radians) - input_z * sinf(yaw_radians);
    world_dz = input_x * sinf(yaw_radians) + input_z * cosf(yaw_radians);

    moving = (input_x != 0.0f || input_z != 0.0f);
    sprinting = sprint_requested && moving && state->stamina > 0.0f;
    move_speed = sprinting ? 3.2f : 2.0f;

    if (sprinting) {
        state->stamina -= 0.55f;
        if (state->stamina < 0.0f) state->stamina = 0.0f;
    } else {
        state->stamina += 0.24f;
        if (state->stamina > 100.0f) state->stamina = 100.0f;
    }

    state->x += world_dx * move_speed;
    state->z += world_dz * move_speed;

    if (state->x < min_x) state->x = min_x;
    if (state->x > max_x) state->x = max_x;
    if (state->z < min_z) state->z = min_z;
    if (state->z > max_z) state->z = max_z;

    return sprinting;
}
