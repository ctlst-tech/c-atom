#include "core_math_vector_calib.h"

 void core_math_vector_calib_exec(
    const core_math_vector_calib_inputs_t *i,
    core_math_vector_calib_outputs_t *o,
    const core_math_vector_calib_params_t *p
)
{
    o->out.x = i->in.x + p->b1;
    o->out.y = i->in.y + p->b2;
    o->out.z = i->in.z + p->b3;
    o->out.x = p->a11 * o->out.x + p->a12 * o->out.y + p->a13 * o->out.z;
    o->out.y = p->a21 * o->out.x + p->a22 * o->out.y + p->a23 * o->out.z;
    o->out.z = p->a31 * o->out.x + p->a32 * o->out.y + p->a33 * o->out.z;
}
