//
// Created by goofy on 9/5/21.
//

#ifndef HW_BRIDGE_FSM_H
#define HW_BRIDGE_FSM_H

#include <stdint.h>

#include "clk.h"
#include "eint/symbol.h"
#define FSM_NAMES_MAX_LENGTH 30

typedef void* fsm_ii_t; //interpreter instructions
typedef symbol_context_t fsm_isc_t; //interpreter symbols_context

typedef enum cond_result {
    cr_true = 1,
    cr_false = 0,
    cr_invalid = -1
} cond_result_t;

/*
typedef cond_result_t (*transition_cond_handler_t)(void *cond_handle);
transition_cond_handler_t cond_handler;
void *cond_data_handle;
*/

typedef struct fsm_entity {
    const char *name; // [FSM_NAMES_MAX_LENGTH + 1];
    const char *annotation;
} fsm_entity_t;

struct state;

typedef struct condition {
    enum {
        cond_expr,
        cond_event
    } type;
} condition_t;

typedef struct transition {
    fsm_entity_t c;

    struct state *to;
    const char *cond_expr_code;
    fsm_ii_t cond_expr_ii;
    const char *transition_code;
    fsm_ii_t transition_ii;

    struct transition * next;
} transition_t;

#define FSM_CLK_SRC_BATCH_SIZE 5

typedef struct clk_src_batch {
    clk_src_t *batch[FSM_CLK_SRC_BATCH_SIZE];
    int num;
} clk_src_batch_t;

struct fsm;

typedef struct state {
    fsm_entity_t c;
    transition_t *active_transitions;
    struct state * next;

    fsm_ii_t on_enter_ii;
    fsm_ii_t on_exit_ii;
    const char *on_enter_code;
    const char *on_exit_code;

    clk_src_batch_t clk_srcs;

    struct fsm *nested_fsm;
} state_t;

typedef struct fsm {
    fsm_entity_t c;

    fsm_isc_t *symbols_context;
    state_t *current_state;
    struct state * states;
    clk_mux_t *clk_mux;

    char *on_update_code;
    fsm_ii_t on_update;

    clk_src_batch_t clk_srcs;

    state_t *begin_state;
    state_t *end_state;

    symbol_t *state_symbol;
} fsm_t;

typedef enum fsm_rv {
    fsm_rv_ok = 0,
    fsm_rv_nomem,
    fsm_rv_invarg,
    fsm_rv_exist,
    fsm_rv_noop,
    fsm_rv_compile,
    fsm_rv_loaderr
} fsm_rv_t;

fsm_rv_t
fsm_create(const char *name, fsm_t **r);

fsm_rv_t
fsm_add_state(fsm_t *fsm, const char *name, state_t **r);

fsm_rv_t
fsm_add_var(fsm_t *fsm, const char *name, const char *type, const char *value);


fsm_rv_t fsm_create_transition(const char *name, const char *cond_expr, struct state *to, transition_t **r);
fsm_rv_t fsm_add_transition(state_t *s, transition_t *t);


state_t *fsm_find_state(fsm_t *f, const char *n);
transition_t *fsm_find_transition(state_t *s, const char *n);
void fsm_print(fsm_t *f);
fsm_rv_t fsm_init_and_compile(fsm_t *f);

fsm_rv_t fsm_assign_clk_mux(fsm_t *f, clk_mux_t *mux);
fsm_rv_t fsm_clk_emit_edge(fsm_t *f, clk_src_t *src);

fsm_rv_t fsm_clk_src_add(fsm_t *f, clk_src_handlers_t *h, void *dh, clk_src_t **src);

//int fsm_update_proc(void *d);
fsm_rv_t fsm_do_update(fsm_t *f);
fsm_rv_t fsm_enter(fsm_t *f);
fsm_rv_t fsm_leave(fsm_t *f);

fsm_rv_t fsm_attach_symbol_context(fsm_t *fsm, fsm_isc_t *symbols_context);

#endif //HW_BRIDGE_FSM_H
