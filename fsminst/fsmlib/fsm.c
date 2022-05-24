//
// Created by goofy on 9/5/21.
//


#include "fsm.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "fsm_interpr.h"
#include "clk.h"

void *my_alloc(size_t s) {
    return calloc(1, s);
}

static state_t * alloc_state () {
    return my_alloc(sizeof(state_t));
}
static transition_t * alloc_transition () {
    return my_alloc(sizeof(transition_t));
}
static fsm_t * alloc_fsm () {
    return my_alloc(sizeof(fsm_t));
}

static fsm_rv_t entity_fillin(fsm_entity_t *e, const char *name, const char *descr) {
    if (e == NULL) {
        return fsm_rv_invarg;
    }

    e->name = name;
//    if (name != NULL) {
//        if (strnlen(name, FSM_NAMES_MAX_LENGTH) >= FSM_NAMES_MAX_LENGTH ) {
//            return fsm_rv_invarg;
//        }
//        strcpy(e->name, name);
//    }

    e->annotation = descr;

    return fsm_rv_ok;
}

typedef void* (*alloc_h)();
#define CAST_TO_ALLOC_HNDL(h__) (alloc_h)(h__)

static fsm_rv_t alloc_and_fillin(alloc_h h, const char *name, void**r) {
    fsm_entity_t *e = (fsm_entity_t *) h();

    if (e == NULL) {
        return fsm_rv_nomem;
    }

    fsm_rv_t rv = entity_fillin(e, name, NULL);
    if (rv != fsm_rv_ok) {
        return rv;
    }

    *r = e;

    return fsm_rv_ok;
}

fsm_rv_t fsm_reset(fsm_t *f) {
    f->current_state = f->begin_state;

    return fsm_rv_ok;
}

fsm_rv_t fsm_create(const char *name, fsm_t **r) {

    fsm_rv_t rv = alloc_and_fillin(CAST_TO_ALLOC_HNDL(alloc_fsm), name, (void **)r);
    if (rv != fsm_rv_ok) {
        return rv;
    }

    fsm_t   *new = *r;

    // TODO check result
    fsm_add_state(*r, "_begin_", &new->begin_state);
    fsm_add_state(*r, "_end_", &new->end_state);

    return fsm_reset(new);
}

#define FSM_NESTED_ITERATE(__fsm, __call, ...) \
    for (state_t *s = __fsm->states; s != NULL; s = s->next) { \
        if (s->nested_fsm != NULL) { \
            fsm_rv_t ___rv; \
            ___rv = __call(s->nested_fsm, __VA_ARGS__); \
            if (___rv != fsm_rv_ok) { \
                return ___rv; \
            } \
        } \
    }

fsm_rv_t fsm_attach_symbol_context(fsm_t *fsm, fsm_isc_t *symbols_context) {
    fsm->symbols_context = symbols_context;

    FSM_NESTED_ITERATE(fsm, fsm_attach_symbol_context, symbols_context);
    return fsm_rv_ok;
}

#define ADD_TO_LIST(l__,nm__,t__) {                     \
    if (l__ != NULL) {                                  \
        t__ *n;                                         \
        for (n = l__; n->next != NULL; n = n->next);    \
        n->next = nm__;                                 \
    } else {                                            \
        l__ = nm__;                                     \
    }                                                   \
}

fsm_rv_t fsm_add_var(fsm_t *fsm, const char *name, const char *type, const char *value) {
    return fsm_isc_add_var (fsm, name, type, value);
}

fsm_rv_t fsm_add_state(fsm_t *fsm, const char *name, state_t **r){
    state_t *new;

    if (fsm == NULL) {
        return fsm_rv_invarg;
    }

    fsm_rv_t rv = alloc_and_fillin(CAST_TO_ALLOC_HNDL(alloc_state), name, (void **)&new);
    if (rv != fsm_rv_ok) {
        return rv;
    }

    ADD_TO_LIST(fsm->states, new, state_t);

    if (r != NULL) {
        *r = new;
    }

    return fsm_rv_ok;
}

fsm_rv_t fsm_create_transition(const char *name,
                               const char *cond_expr,
                               struct state *to,
                                       transition_t **r) {

    transition_t *new;

    if ((to == NULL) || (cond_expr == NULL)) {
        return fsm_rv_invarg;
    }

    fsm_rv_t rv = alloc_and_fillin(CAST_TO_ALLOC_HNDL(alloc_transition), name, (void **)&new);
    if (rv != fsm_rv_ok) {
        return rv;
    }

    new->cond_expr_code = cond_expr;
    new->to = to;

    if (r != NULL) {
        *r = new;
    }

    return fsm_rv_ok;
}

fsm_rv_t fsm_add_transition(state_t *s, transition_t *t) {

    ADD_TO_LIST(s->active_transitions, t, transition_t);

    return fsm_rv_ok;
}

#define FIND(elmt_t__)                                          \
/*elmt_t__ *fsm_find_state(elmt_t__ *l, const char *sname) { */ \
    elmt_t__ *n;                                                \
    for (n = l; n != NULL; n = n->next) {                       \
        if (!strcmp(n->c.name, name)) {                         \
            return n;                                           \
        }                                                       \
    }                                                           \
    return NULL;


static state_t *fsm_find_state__(state_t *l, const char *name) {
    FIND(state_t);
}

static transition_t *fsm_find_transition__(transition_t *l, const char *name) {
    FIND(transition_t);
}

state_t *fsm_find_state(fsm_t *f, const char *n) {
    return fsm_find_state__(f->states, n);
}

transition_t *fsm_find_transition(state_t *s, const char *n) {
    return fsm_find_transition__(s->active_transitions, n);
}

static void remove_spaces(const char* s, char *rv) {
    const char *i = s;

    if (s == NULL) {
        return;
    }

    int loop;
    do {
        do {
            loop = 1;
            switch (*i) {
                case ' ':
                case '\r':
                case '\n':
                case '\t':
                    i++;
                    break;
                default:
                    loop = 0;
            }
        } while(loop);
        *rv++ = *i++;

    } while (*i);
}

void fsm_transition_print(transition_t *t) {
    // TODO handle size
    char ec[255]="";
    char tc[255]="";

    remove_spaces(t->cond_expr_code, ec);
    remove_spaces(t->transition_code, tc);

    printf("        Transition \"%s\" to state \"%s\"\n", t->c.name, t->to->c.name);
    if (ec[0]) printf("          - Triggering expr \"%s\"\n", ec );
    if (tc[0]) printf("          - Action on transition \"%s\"\n", tc );
}

void fsm_state_print(state_t *s) {
    char enc[255]="";
    char exc[255]="";

    remove_spaces(s->on_enter_code, enc);
    remove_spaces(s->on_exit_code, exc);

    printf("    State \"%s\"\n", s->c.name);
    if (enc[0]) printf("     - Action on enter \"%s\"\n", enc);
    if (exc[0]) printf("     - Action on exit  \"%s\"\n", exc);
}

void fsm_print(fsm_t *f) {

    printf ("fsm \"%s\"\n", f->c.name);
    for (state_t *n = f->states; n != NULL; n=n->next) {
        fsm_state_print(n);
        for (transition_t *m = n->active_transitions; m != NULL; m = m->next) {
            fsm_transition_print(m);
        }
    }
}

fsm_rv_t fsm_actualize_state_var(fsm_t *f) {
    int rv = symbol_update_w_str(f->state_symbol, f->current_state->c.name);
    return rv ? fsm_rv_invarg : fsm_rv_ok;
}


fsm_rv_t fsm_init(fsm_t *f, int dummy) {

    f->state_symbol = symbol_add_var(f->symbols_context, f->c.name, nr_string);
    if (f->state_symbol == NULL) {
        return fsm_rv_nomem;
    }

    fsm_actualize_state_var(f);

    FSM_NESTED_ITERATE(f, fsm_init, 0);

    return fsm_rv_ok;
}

fsm_rv_t fsm_compile(fsm_t *f, int dummy) {

#define CASTII (fsm_ii_t **)
#define ASSERT_ERR(__call) {if ((__call) == fsm_rv_compile) {return fsm_rv_compile;}}
    ASSERT_ERR(fsm_ii_compile(&f->on_update, f->symbols_context, f->on_update_code));

    for (state_t *n = f->states; n != NULL; n=n->next) {
        ASSERT_ERR(fsm_ii_compile(&n->on_enter_ii, f->symbols_context, n->on_enter_code));
        ASSERT_ERR(fsm_ii_compile(&n->on_exit_ii, f->symbols_context, n->on_exit_code));
        for (transition_t *m = n->active_transitions; m != NULL; m = m->next) {
            ASSERT_ERR(fsm_ii_compile(&m->cond_expr_ii, f->symbols_context, m->cond_expr_code));
            ASSERT_ERR(fsm_ii_compile(&m->transition_ii, f->symbols_context, m->transition_code));
        }
    }


    FSM_NESTED_ITERATE(f, fsm_compile, 0);

    return fsm_rv_ok;
}

fsm_rv_t fsm_init_and_compile(fsm_t *f) {
    fsm_rv_t rv = fsm_init(f, 0);
    if (rv != fsm_rv_ok) {
        return rv;
    }

    return fsm_compile(f, 0);
}

// TODO some clk is because of transfer, and some are for the whole FSM.

fsm_rv_t fsm_assign_clk_mux(fsm_t *f, clk_mux_t *mux) {
    f->clk_mux = mux;

    return fsm_rv_ok;
}

//fsm_rv_t fsm_clk_emit_edge(fsm_t *f, clk_src_t *src) {
//
//
//    return fsm_rv_ok;
//}

fsm_rv_t fsm_clk_src_add(fsm_t *f, clk_src_handlers_t *h, void *dh, clk_src_t **src) {

    switch (clk_src_add(f->clk_mux, h, dh, src)) {
        default:
        case clk_rv_nomem:
            return fsm_rv_nomem;

        case clk_rv_ok:
            return fsm_rv_ok;
    }
}

inline static void fsm_clk_srcs_toggle(clk_src_batch_t *b, int enable) {
    int i;
    clk_src_t **src = b->batch;
    clk_rv_t (*func)(clk_src_t *) = (enable ? clk_src_enable : clk_src_disable);

    for (i = 0; i < b->num; i++) {
        func(src[i]);
    }
}

static void fsm_clk_srcs_disable(clk_src_batch_t *b) {
    fsm_clk_srcs_toggle(b, 0);
}

static void fsm_clk_srcs_enable(clk_src_batch_t *b) {
    fsm_clk_srcs_toggle(b, 1);
}

static int activate_transitions(state_t *s) {
    for (transition_t *t = s->active_transitions; t != NULL; t = t->next) {
        if (t->transition_ii != NULL) {
            fsm_ii_update_symbols_state(t->transition_ii);
        }
        if (t->cond_expr_ii != NULL) {
            fsm_ii_update_symbols_state(t->cond_expr_ii);
        }
    }
    return fsm_rv_ok;
}

static int fsm_transition(fsm_t *f, transition_t *firing_trans) {

    printf ("%s | %s  from %s to %s\n", __func__, f->c.name, f->current_state->c.name, firing_trans->to->c.name );


    // TODO process exit from the nested state


    fsm_ii_action_exec(f->current_state->on_exit_ii);
    fsm_ii_action_exec(firing_trans->transition_ii);
    fsm_clk_srcs_disable(&f->current_state->clk_srcs);
    f->current_state = firing_trans->to; // TODO arrange reset activities to be more explicit and encapsulated
    if(f->current_state->nested_fsm != NULL) {
        fsm_reset(f->current_state->nested_fsm);
    }
    activate_transitions(f->current_state);
    fsm_ii_action_exec(f->current_state->on_enter_ii);
    fsm_clk_srcs_enable(&f->current_state->clk_srcs);
    fsm_actualize_state_var(f);

    return 0;
}

fsm_rv_t fsm_enter(fsm_t *f) {
    fsm_clk_srcs_enable(&f->clk_srcs);
    return fsm_rv_ok;
}

fsm_rv_t fsm_leave(fsm_t *f) {
    fsm_clk_srcs_disable(&f->clk_srcs);
    return fsm_rv_ok;
}

fsm_rv_t fsm_do_update(fsm_t *f) {
    transition_t *t;
    int rslt;
    fsm_rv_t rv;

    if (f->current_state == f->end_state) {
        return fsm_rv_noop;
    }

    fsm_ii_action_exec(f->on_update);

    for (t = f->current_state->active_transitions; t != NULL; t = t->next) {
        rv = fsm_ii_exec(t->cond_expr_ii, &rslt);
        if ((rv == fsm_rv_ok) && rslt) {
            fsm_transition(f, t);
            break; // one transition per iteration
        }
    }

    if (f->current_state->nested_fsm != NULL) {
        fsm_do_update(f->current_state->nested_fsm);
    }

    return fsm_rv_ok;
}

int fsm_update_proc(void *d) {
    fsm_t *f = (fsm_t*) d;
    return fsm_do_update(f);
}

/*
fsm_rv_t fsm_process(fsm_t *f) {

    clk_mux_t m;
    clk_mux_driver_t *dr = NULL; // TODO
    clk_init(&m, dr, NULL, fsm_update_, f);

    clk_cycle(&m);
    // never returns actually
    return fsm_rv_ok;
}
*/
