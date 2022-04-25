#include "core_math_inverse_gain.h"

void core_math_inverse_gain_compute_params(
    core_math_inverse_gain_params_t *old_p,
    core_math_inverse_gain_params_t *new_p,
    core_math_inverse_gain_params_flags_t flags
)
{
    new_p->gain = 1.0 / new_p->inverse_gain;
}
