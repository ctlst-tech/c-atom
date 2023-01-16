#include "core_cont_mux.h"

void core_cont_mux_exec(const core_cont_mux_inputs_t *i, core_cont_mux_outputs_t *o)
{
    o->output = i->select ? i->input1 : i->input0;
}
