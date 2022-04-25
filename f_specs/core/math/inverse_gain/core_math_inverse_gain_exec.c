#include "core_math_inverse_gain.h"

void core_math_inverse_gain_exec(
    const core_math_inverse_gain_inputs_t *i,
    core_math_inverse_gain_outputs_t *o,
    const core_math_inverse_gain_params_t *p
)
{
    o->output = i->input * p->inverse_gain;
}
