#include <math.h>

#include "core_quat_from_euler.h"

void core_quat_from_euler_exec(const core_quat_from_euler_inputs_t *i, core_quat_from_euler_outputs_t *o)
{
    float sin_roll	= sinf(i->roll / 2 );
    float cos_roll	= cosf(i->roll / 2 );
    float sin_pitch= sinf(i->pitch / 2 );
    float cos_pitch= cosf(i->pitch / 2 );
    float cos_yaw	= cosf(i->yaw / 2 );
    float sin_yaw	= sinf(i->yaw / 2 );

    o->q.w = cos_roll * cos_pitch * cos_yaw + sin_roll * sin_pitch * sin_yaw;
    o->q.x = sin_roll * cos_pitch * cos_yaw - cos_roll * sin_pitch * sin_yaw;
    o->q.y = cos_roll * sin_pitch * cos_yaw + sin_roll * cos_pitch * sin_yaw;
    o->q.z = cos_roll * cos_pitch * sin_yaw - sin_roll * sin_pitch * cos_yaw;

    // TODO cascade update e.g. for initial setup of the quat
}
