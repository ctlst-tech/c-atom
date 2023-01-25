#include "core_vector_mul_scalar.h"

 void core_vector_mul_scalar_exec(const core_vector_mul_scalar_inputs_t *i, core_vector_mul_scalar_outputs_t *o)
{
    o->v.x = i->v.x * i->scalar;
    o->v.y = i->v.y * i->scalar;
    o->v.z = i->v.z * i->scalar;
}

