#ifndef HW_BRIDGE_EINT_H
#define HW_BRIDGE_EINT_H

#include "symbol.h"
#include "instr.h"

typedef struct symbol_state_handler_callback {
    void *state_ref;
    symbol_state_handler_t *sh;
    struct symbol_state_handler_callback *next;
} symbol_state_handler_call_item_t;

typedef struct eint_instance {
    //parser_lexer_flex_bison_t lexer_parser_cfg;
    symbol_context_t *context_ref;
    struct ast_node *ast_root;
    symbol_state_handler_call_item_t *shc_list;
    instr_queue_t instr_queue;
} eint_instance_t;

int eint_init_and_compile(eint_instance_t *ei, symbol_context_t *cntx, const char *str2parse);
result_reg_t *eint_exec(eint_instance_t *i);
result_type_t eint_parse_type(const char *type);
const char *eint_print_type_result(result_type_t t);
const char *eint_print_result(result_reg_t *r);
void eint_print_all_vars(struct ast_node *n);

int eint_test_all ();

#endif //HW_BRIDGE_EINT_H
