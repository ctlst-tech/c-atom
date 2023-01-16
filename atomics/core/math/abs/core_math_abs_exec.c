#include "core_math_abs.h"

#include <math.h>

void core_math_abs_exec(
    const core_math_abs_inputs_t *i,
    core_math_abs_outputs_t *o
)
{
    o->output = fabs(i->input);
}
