#include "core_source_sin.h"
#include "error.h"
#include <math.h>

void core_source_sin_compute_params(
    core_source_sin_params_t *old_p,
    core_source_sin_params_t *new_p,
    core_source_sin_params_flags_t flags
)
{
    new_p->omega = (1.0 / new_p->period) * M_2_PI;
}
