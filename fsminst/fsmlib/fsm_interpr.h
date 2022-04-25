//
// Created by goofy on 9/14/21.
//

#ifndef HW_BRIDGE_FSM_INTERPR_H
#define HW_BRIDGE_FSM_INTERPR_H

#include "fsm.h"

fsm_rv_t fsm_isc_allocate(fsm_t *fsm);
fsm_rv_t fsm_isc_add_var (fsm_t *fsm, const char *name, const char *type, const char *value);

fsm_rv_t fsm_ii_compile(fsm_ii_t *ii, fsm_isc_t *isc, const char *script);
fsm_rv_t fsm_ii_exec(fsm_ii_t ii, int *result);
fsm_rv_t fsm_ii_action_exec(fsm_ii_t ii);

fsm_rv_t fsm_ii_update_symbols_state(fsm_ii_t ii);


#endif //HW_BRIDGE_FSM_INTERPR_H
