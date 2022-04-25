#include "core_controller_mux.h"

void core_controller_mux_exec(const core_controller_mux_inputs_t *i, core_controller_mux_outputs_t *o)
{
    o->output = i->select ? i->input1 : i->input0;
}
