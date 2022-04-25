//
// Created by goofy on 9/4/21.
//

#ifndef HW_BRIDGE_AST_H
#define HW_BRIDGE_AST_H

#include <stdint.h>
#include "eint_types.h"


typedef enum {
    ast_void = 0,
    ast_relational_g,
    ast_relational_ge,
    ast_relational_l,
    ast_relational_le,
    ast_arithmetic_minus,
    ast_arithmetic_plus,
    ast_arithmetic_div,
    ast_arithmetic_mul,
    ast_arithmetic_unary_min,
    ast_logic_unary_not,
    ast_logic_or,
    ast_logic_and,
    ast_equality_equal,
    ast_equality_not_equal,
    ast_function,
    ast_symbol_ref,
    ast_assignment,
    ast_number_int,
    ast_number_float,
    ast_bool_const,
    ast_str_const,
    ast_list,

    ast_statement,
} ast_node_type_t;



typedef enum {
    nc_binary,
    nc_unary,
    nc_reference,
    nc_flow,
    nc_const
} ast_node_class_t;


struct symbol;
struct result_reg;

struct ast_node {
    ast_node_type_t node_type;

    struct ast_node * left;
    struct ast_node * right;

    int dot_draw_num;

    union {
//        struct  {
//            struct symbol * symbol;
//            //struct ast_node * arguments; // must be equal to left node
//        } function;
        struct  {
            struct symbol * symbol;
        } sybmol_reference;
//        struct {
//            struct symbol * symbol;
//            //struct ast_node * value; // must be equal to left node
//        } assigment;
        struct {
            double value;
        } number_float;
        struct {
            int32_t value;
        } number_int;
        struct {
            boolval_t value;
        } bool_const;
        struct {
            const char *value;
        } str_const;
    };

    result_reg_t *res_reg;     // TODO must be in a separate tree?
};

struct ast_node *new_ast_node (ast_node_type_t node_type,
              struct ast_node * left,
              struct ast_node * right);

struct ast_node *new_ast_statement_node(struct ast_node *stmnt_l, struct ast_node *stmnt_r);

struct ast_node *new_ast_function_node (struct symbol * symbol,
                       struct ast_node * arguments);

struct ast_node *new_ast_symbol_reference_node (struct symbol * symbol);

struct ast_node *new_ast_assignment_node (struct symbol * symbol,
                         struct ast_node * value);

struct ast_node *new_ast_number_float_node (double value);

struct ast_node *new_ast_number_int_node (int value);

struct ast_node *new_ast_bool_const_node (int value);
struct ast_node *new_ast_str_const_node (const char *value);

int print_ast_to_file (const char *filename, struct ast_node *n);

const char * ast_node_type_caption (ast_node_type_t t);
const char * ast_node_full_caption(struct ast_node *n);;

ast_node_class_t ast_node_class(struct ast_node *n);

#endif //HW_BRIDGE_AST_H
