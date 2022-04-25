#include "core_math_value_limiter.h"

void core_math_value_limiter_exec(
    const core_math_value_limiter_inputs_t *i,
    core_math_value_limiter_outputs_t *o,
    const core_math_value_limiter_params_t *p
)
{
    if (i->input < p->min) {
        o->output = p->min;
    } else if (i->input > p->max) {
        o->output = p->max;
    } else {
        o->output = i->input;
    }
}
