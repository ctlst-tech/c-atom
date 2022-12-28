#include "core_cont_mode_switch2.h"

void core_cont_mode_switch2_exec(const core_cont_mode_switch2_inputs_t *i, core_cont_mode_switch2_outputs_t *o){
    o->mode1 =  i->input > 0.3 ? TRUE : FALSE;
    o->mode2 =  i->input > 0.7 ? TRUE : FALSE;
}
