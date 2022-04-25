#include "core_math_mul.h"

void core_math_mul_exec(
    const core_math_mul_inputs_t *i,
    core_math_mul_outputs_t *o
)
{
    o->output = (i->input0) * (i->input1);
}
