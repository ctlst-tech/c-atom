#include "core_source_constant.h"

void core_source_constant_exec(
    core_source_constant_outputs_t *o,
    const core_source_constant_params_t *p
)
{
    o->output = p->value;
}
