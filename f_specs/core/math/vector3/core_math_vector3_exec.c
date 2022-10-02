#include "core_math_vector3.h"

void core_math_vector3_exec(const core_math_vector3_inputs_t *i, core_math_vector3_outputs_t *o)
{
    o->v.x = i->x;
    o->v.y = i->y;
    o->v.z = i->z;
}

