//
// Created by goofy on 9/14/21.
//

#include <stddef.h>
#include <stdlib.h>
#include "fsm_interpr.h"
#include "eint.h"

fsm_rv_t fsm_isc_allocate(fsm_t *fsm) {
    // TODO avoid malloc
    fsm->symbols_context = calloc(1, sizeof(symbol_context_t));

    return fsm_rv_ok;
}

// TODO script allowed instructions and operands
fsm_rv_t fsm_ii_compile(fsm_ii_t *ii, fsm_isc_t *isc, const char *script) {

    if (script == NULL) {
        // it is ok not to have a script;
        return fsm_rv_ok;
    }

    if (ii == NULL) {
        return fsm_rv_invarg;
    }

    // TODO avoid malloc
    eint_instance_t *e = calloc(1, sizeof(eint_instance_t));
    symbol_context_t *c = (symbol_context_t *) isc;

    if (eint_init_and_compile(e, c, script)) {
        if (e->ast_root == NULL) {
            *ii = NULL; // TODO parse errors correctly act accordingly
            return fsm_rv_ok;
        }
        return fsm_rv_compile;
    }

    *ii = e;

    return fsm_rv_ok;
}

fsm_rv_t fsm_ii_exec(fsm_ii_t ii, int *result) {
//    if (!ii) {
//        return fsm_rv_noop;
//    } TODO let it crash. Condition must have a expressions

    eint_instance_t *e = (eint_instance_t*) ii;

    //eint_print_all_vars(e->root);

    result_reg_t *rr = eint_exec(e);
    *result = rr->val.b ? -1 : 0;

    return fsm_rv_ok;
}

fsm_rv_t fsm_ii_update_symbols_state(fsm_ii_t ii) {
    eint_instance_t *e = (eint_instance_t*) ii;

    for (symbol_state_handler_call_item_t *n = e->shc_list; n != NULL; n = n->next) {
        if (n->sh->state_activate != NULL) {
            n->sh->state_activate(n->state_ref);
        }
    }

    return fsm_rv_ok;
}

fsm_rv_t fsm_ii_action_exec(fsm_ii_t ii) {
    if (!ii) {
        return fsm_rv_noop;
    }
    eint_instance_t *e = (eint_instance_t*) ii;

    eint_exec(e);

    return fsm_rv_ok;
}

fsm_rv_t fsm_isc_add_var (fsm_t *fsm, const char *name, const char *type, const char *value) {

    result_type_t t = eint_parse_type(type);

    if (t == nr_na) {
        return fsm_rv_invarg;
    }

    symbol_t *s = symbol_lookup(fsm->symbols_context, name);
    if (s == NULL) {
        return fsm_rv_nomem;
    }

    if ((s->type != symt_noref) && (s->i.var.type != nr_na)) {
        return fsm_rv_exist;
    }

    s->type = symt_var;
    s->i.var.type = t;

    int v;
    double d;

    symbol_update_w_str(s, value);

    return fsm_rv_ok;
}
