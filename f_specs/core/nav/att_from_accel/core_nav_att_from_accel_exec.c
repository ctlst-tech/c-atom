#include <math.h>
#include "core_nav_att_from_accel.h"

#define square(__a) ((__a)*(__a))

void core_nav_att_from_accel_exec(const core_nav_att_from_accel_inputs_t *i, core_nav_att_from_accel_outputs_t *o)
{
    o->roll = atan2(i->a.y, i->a.z);
    o->pitch = atan2(-i->a.x, sqrt(square(i->a.z) + square(i->a.y)));
}
