#include "core_math_cos.h"

#include <math.h>

void core_math_cos_exec(
    const core_math_cos_inputs_t *i,
    core_math_cos_outputs_t *o
)
{
    o->output = cos(i->input);
}
