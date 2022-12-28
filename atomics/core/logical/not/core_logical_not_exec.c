#include "core_logical_not.h"

void core_logical_not_exec(
    const core_logical_not_inputs_t *i,
    core_logical_not_outputs_t *o
)
{
    o->output = !(i->input);
}
