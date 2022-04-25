#include "core_math_sin.h"

#include <math.h>

void core_math_sin_exec(
    const core_math_sin_inputs_t *i,
    core_math_sin_outputs_t *o
)
{
    o->output = sin(i->input);
}
