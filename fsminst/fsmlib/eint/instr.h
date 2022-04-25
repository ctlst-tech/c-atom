//
// Created by goofy on 9/7/21.
//

#ifndef HW_BRIDGE_INSTR_H
#define HW_BRIDGE_INSTR_H

#include <stdint.h>
#include "eint_types.h"
#include "symbol.h"
//#include "eint.h"

typedef void (*instr_handler_t)(void *l, void *r, void *res) ;

typedef struct {
    void *l_operand;
    void *r_operand;
    void *res;
    //result_val_t *result;
    //result_type_t rslt_type;

    result_reg_t *res_reg_ref;

    instr_handler_t handler;

    // dbg
    struct ast_node *node;
} instr_t;

#define EINT_MAX_INSTR 20
#define EINT_MAX_REGS 20

typedef struct {
    int reg_num;
    int istr_num;
    result_reg_t registers[EINT_MAX_REGS];
    instr_t instructions[EINT_MAX_INSTR];

    result_reg_t *res_reg_ref;

    boolval_t compiled;

} instr_queue_t;

struct eint_instance;

result_reg_t *instr_queue_exec(instr_queue_t *q);
int instr_compile_ast(struct eint_instance *ei);

#endif //HW_BRIDGE_INSTR_H
