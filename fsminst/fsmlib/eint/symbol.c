//
// Created by goofy on 9/5/21.
//

#include <string.h>

#include "symbol.h"

void *my_alloc(ssize_t s);

static symbol_t * alloc_sybmol () {
    return my_alloc(sizeof(symbol_t));
}
static symbol_t * create_sybmol (const char *name) {
    symbol_t *s;
    if (strnlen(name, SYMBOL_NAME_MAX_LENGTH) >= SYMBOL_NAME_MAX_LENGTH) {
        // TODO errno
        return NULL;
    }

    s = alloc_sybmol();
    strcpy(s->name, name);
    s->type = symt_noref;

    return s;
}

symbol_t *symbol_find_and_create(symbol_context_t *cntx, const char *name, int reg_new) {
    symbol_t *n, *p = NULL;
    for (n = cntx->symbols; n != NULL; n = n->next) {
        if (strncmp(name, n->name, SYMBOL_NAME_MAX_LENGTH) == 0) {
            return n;
        }
        p = n;
    }

    if (reg_new) {
        symbol_t *new_sig = create_sybmol(name);
        //printf ("%s, new symbol %s\n", __func__, name);
        if (p != NULL) {
            p->next = new_sig;
        } else {
            cntx->symbols = new_sig;
        }
        return new_sig;
    }

    return NULL;
}

/**
 * Allocates symbol if it does not exits
 * @param cntx
 * @param name
 * @return
 */
symbol_t *symbol_lookup(symbol_context_t *cntx, const char *name) {
    return symbol_find_and_create(cntx, name, -1);
}

symbol_t *symbol_find(symbol_context_t *cntx, const char *name) {
    return symbol_find_and_create(cntx, name, 0);
}

const char *eint_print_type_result(result_type_t t);
const char *eint_print_result(result_reg_t *r);

void symbol_func_print_args(symbol_func_args_list_t *al) {
    int i;

    printf("Argument list size: %d. Arguments list:\n", al->args_num);
    for (i=0; i < al->args_num; i++) {
        printf("arg %d value %s type %s\n", i+1, eint_print_result(al->args[i]), eint_print_type_result(al->args[i]->type));
    }
}

symbol_t *symbol_add_func(symbol_context_t *cntx, const char *name, symbol_func_handler_t h, symbol_state_handler_t *sh, int expected_args_num,
                          result_type_t ret_type) {
    symbol_t *rv;

    rv = symbol_lookup(cntx, name);
    if (rv == NULL) {
        return rv;
    }

    rv->type = symt_func;
    rv->i.func.handler = h;
//    rv->i.func.data.val.p = sh;
//    if (sh != NULL) rv->i.func.data.type = nr_ptr;
    rv->i.func.state_handler = sh;
    rv->i.func.ret_val_type = ret_type;
    rv->i.func.expected_args_num = expected_args_num;

    return rv;
}

#define SYMBOL_MAX_STRING_LNG 200

symbol_t *symbol_add_var(symbol_context_t *cntx, const char *name, result_type_t rt) {
    symbol_t *rv;

    rv = symbol_lookup(cntx, name);
    if (rv == NULL) {
        return rv;
    }

    rv->type = symt_var;
    rv->i.var.type = rt;
    if (rt == nr_string) {
        rv->i.var.val.s = my_alloc(SYMBOL_MAX_STRING_LNG);
    }

    return rv;
}

void symbol_update_float(symbol_t *s, double v){
    // TODO check types
    s->i.var.val.f = v;
}

void symbol_update_int(symbol_t *s, int v){
    // TODO check types
    s->i.var.val.i = v;
}

void symbol_update_bool(symbol_t *s, int v){
    // TODO check types
    s->i.var.val.b = v;
}



void symbol_update(symbol_t *s, void *value) {
    switch (s->i.var.type) {
        case nr_float:
            s->i.var.val.f =  *(double*)value;
            break;

        case nr_int:
            s->i.var.val.i =  *(int*)value;
            break;

        case nr_bool:
            s->i.var.val.b =  *(int*)value;
            break;

        case nr_string:
            strncpy(s->i.var.val.s, (const char*)value, SYMBOL_MAX_STRING_LNG);
            break;

        default:
            break;
    }
}

#include <stdlib.h>

static boolval_t check_bool(const char *v) {
    if ((strcasecmp(v, "1") == 0) || (strcasecmp(v, "true") == 0)
     || (strcasecmp(v, "yes") == 0)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

#include "../../function/conv.h"

/**
 *
 * @param s
 * @param value
 * @return
 */
int symbol_update_w_str(symbol_t *s, const char *value) {
    int rv = 0;

    if (s->type == symt_var) {
        switch (s->i.var.type) {
            case nr_float:
                rv = conv_str_double(value, &s->i.var.val.f) == conv_rv_ok ? 1 : 0;
                break;

            case nr_int:
                rv = sscanf(value, "%d", &s->i.var.val.i);
                break;

            case nr_bool:
                s->i.var.val.b = check_bool(value);
                rv = 1;
                break;

            case nr_string:
                strncpy(s->i.var.val.s, value, SYMBOL_MAX_STRING_LNG);
                rv = 1;
                break;

            default:
                rv = 0;
                break;
        }
    }

    return rv == 1 ? 0 : 1;
}
