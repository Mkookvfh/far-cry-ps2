#include "player_collision.h"
#include <math.h>

typedef struct {
    float min_x, max_x, min_z, max_z;
} CollisionBox;

static const CollisionBox buildings[] = {
    /* Village house near the western trail. */
    {166.0f, 210.0f, 158.0f, 199.0f},
    /* Eastern village house. */
    {399.0f, 447.0f, 246.0f, 294.0f},
    /* Landmark tower. */
    {493.0f, 507.0f, 183.0f, 197.0f}
};

float island_ground_height(float x, float z)
{
    float nx = (x - 320.0f) / 218.0f;
    float nz = (z - 224.0f) / 164.0f;
    float d = nx * nx + nz * nz;
    float hill = 24.0f * (1.0f - d);
    float peak_dx = (x - 310.0f) / 76.0f;
    float peak_dz = (z - 148.0f) / 58.0f;
    float peak = 17.0f * expf(-(peak_dx * peak_dx + peak_dz * peak_dz));

    if (d > 1.0f) return -2.5f;
    if (hill < 0.0f) hill = 0.0f;
    return 1.5f + hill + peak;
}

int island_is_land(float x, float z)
{
    float nx = (x - 320.0f) / 218.0f;
    float nz = (z - 224.0f) / 164.0f;
    return (nx * nx + nz * nz) <= 0.96f;
}

static int collides_building(float x, float z, float radius)
{
    unsigned int i;
    for (i = 0; i < sizeof(buildings) / sizeof(buildings[0]); ++i) {
        const CollisionBox *b = &buildings[i];
        if (x + radius > b->min_x && x - radius < b->max_x &&
            z + radius > b->min_z && z - radius < b->max_z)
            return 1;
    }
    return 0;
}

static int position_is_blocked(float x, float z, float radius)
{
    return !island_is_land(x, z) || collides_building(x, z, radius);
}

void island_resolve_player(PlayerMoveState *state,
                           float previous_x, float previous_z,
                           float radius)
{
    float candidate_x, candidate_z;

    if (state == 0) return;
    if (radius < 0.0f) radius = 0.0f;

    candidate_x = state->x;
    candidate_z = state->z;

    /* Keep valid motion; otherwise attempt axis sliding along the obstacle. */
    if (!position_is_blocked(candidate_x, candidate_z, radius))
        return;

    if (!position_is_blocked(candidate_x, previous_z, radius)) {
        state->z = previous_z;
        return;
    }

    if (!position_is_blocked(previous_x, candidate_z, radius)) {
        state->x = previous_x;
        return;
    }

    state->x = previous_x;
    state->z = previous_z;
}
