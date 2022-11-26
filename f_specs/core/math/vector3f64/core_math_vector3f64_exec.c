#include "core_math_vector3f64.h"

void core_math_vector3f64_exec(const core_math_vector3f64_inputs_t *i, core_math_vector3f64_outputs_t *o)
{
    o->v.x = i->x;
    o->v.y = i->y;
    o->v.z = i->z;
}
