#include "core_vector_calib.h"

 void core_vector_calib_exec(
    const core_vector_calib_inputs_t *i,
    core_vector_calib_outputs_t *o,
    const core_vector_calib_params_t *p
)
{
    o->v.x = i->v.x + p->b1;
    o->v.y = i->v.y + p->b2;
    o->v.z = i->v.z + p->b3;
    o->v.x = p->a11 * o->v.x + p->a12 * o->v.y + p->a13 * o->v.z;
    o->v.y = p->a21 * o->v.x + p->a22 * o->v.y + p->a23 * o->v.z;
    o->v.z = p->a31 * o->v.x + p->a32 * o->v.y + p->a33 * o->v.z;
}

