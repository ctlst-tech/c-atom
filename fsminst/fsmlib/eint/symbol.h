//
// Created by goofy on 9/5/21.
//

#ifndef HW_BRIDGE_SYMBOL_H
#define HW_BRIDGE_SYMBOL_H

#include <stdint.h>
#include <stdio.h>
#include "eint_types.h"

#define SYMBOL_NAME_MAX_LENGTH 50

struct symbol;

#define SYMBOL_FUNC_MAX_ARGS 6
typedef struct {
    int args_num;
    result_reg_t *args[SYMBOL_FUNC_MAX_ARGS];
} symbol_func_args_list_t;

typedef void (*symbol_func_handler_t)(symbol_func_args_list_t *, result_val_t *);

typedef int (*symbol_state_handler_construct_t)(void *dhandle, void **state_ref);
typedef int (*symbol_state_handler_activate_t)(void *state_ref);

typedef struct symbol_state_handler {
    void *dhandle;
    symbol_state_handler_construct_t state_constructor;
    symbol_state_handler_activate_t state_activate; // called at activation of transition/inst set
} symbol_state_handler_t;


typedef struct symbol {
    char name[SYMBOL_NAME_MAX_LENGTH + 1];
    enum {
        symt_noref = 0,
        symt_var,
        symt_func
    } type;

    union {
        result_reg_t var;
        struct {
            result_type_t ret_val_type;
            symbol_state_handler_t *state_handler;
            int expected_args_num;
            symbol_func_handler_t handler;
        } func;
    } i;
    struct symbol *next;
} symbol_t;

typedef struct symbol_context {
    symbol_t    *symbols;
} symbol_context_t;




symbol_t *symbol_lookup(symbol_context_t *cntx, const char *name);
symbol_t *symbol_find(symbol_context_t *cntx, const char *name);

void symbol_func_print_args(symbol_func_args_list_t *al);

symbol_t *symbol_add_var(symbol_context_t *cntx, const char *name, result_type_t rt);
symbol_t *symbol_add_func(symbol_context_t *cntx, const char *name, symbol_func_handler_t h, symbol_state_handler_t *sh, int expected_args_num,
                          result_type_t ret_type);

void symbol_update(symbol_t *s, void *value);
int symbol_update_w_str(symbol_t *s, const char *value);

#endif //HW_BRIDGE_SYMBOL_H
