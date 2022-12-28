#include "core_cont_mux3f64.h"

void core_cont_mux3f64_exec(const core_cont_mux3f64_inputs_t *i, core_cont_mux3f64_outputs_t *o)
{
    o->output = i->select ? i->input1 : i->input0;
}
