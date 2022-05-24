//
// Created by goofy on 9/4/21.
//

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <stddef.h>
#include <string.h>

#include "ast.h"
#include "symbol.h"

void *
emalloc (size_t size)
{
    void * pointer = calloc (1, size);

    if (!pointer)
        fprintf (stderr, "Error: malloc(%zu) failed!\n", size);

    return pointer;
}




//#define AST_DEBUG(n, l,r)  {printf ("%s node_type=%d left == %p right == %p\n",__func__, ast_node->node_type, (void *)l, (void *)r);}

#define AST_DEBUG(n)  {printf ("%s node_caption=%s\n",__func__, node_full_caption(n));}

struct ast_node *new_ast_node (ast_node_type_t node_type,
              struct ast_node * left,
              struct ast_node * right)
{
    struct ast_node * ast_node =
            emalloc (sizeof (struct ast_node));

    ast_node->node_type = node_type;

    ast_node->left = left;
    ast_node->right = right;

    //AST_DEBUG(ast_node);

    return ast_node;
}


struct ast_node *new_ast_function_node (struct symbol * symbol,
                       struct ast_node * arguments)
{
    struct ast_node * nn = new_ast_node(ast_function, arguments, NULL);
    nn->sybmol_reference.symbol = symbol;
    return (struct ast_node *) nn;
}

struct ast_node *new_ast_statement_node(struct ast_node *stmnt_l, struct ast_node *stmnt_r)
{
    struct ast_node * nn = new_ast_node(ast_statement, stmnt_l, stmnt_r);
    return (struct ast_node *) nn;
}

struct ast_node *new_ast_symbol_reference_node (struct symbol * symbol)
{
    struct ast_node * nn = new_ast_node(ast_symbol_ref, NULL, NULL);
    nn->sybmol_reference.symbol = symbol;
    return (struct ast_node *) nn;
}


struct ast_node *new_ast_number_float_node (double value)
{
    struct ast_node * nn = new_ast_node(ast_number_float, NULL, NULL);
    nn->number_float.value = value;
    return (struct ast_node *) nn;
}

struct ast_node *new_ast_number_int_node (int value)
{
    struct ast_node * nn = new_ast_node(ast_number_int, NULL, NULL);
    nn->number_int.value = value;
    return (struct ast_node *) nn;
}

struct ast_node *new_ast_bool_const_node (boolval_t value)
{
    struct ast_node * nn = new_ast_node(ast_bool_const, NULL, NULL);
    nn->bool_const.value = value;
    return (struct ast_node *) nn;
}

struct ast_node *new_ast_str_const_node (const char *value)
{
    struct ast_node * nn = new_ast_node(ast_str_const, NULL, NULL);
    nn->str_const.value = strdup(value);
    return (struct ast_node *) nn;
}

struct ast_node *new_ast_assignment_node (struct symbol * symbol,
                         struct ast_node * value)
{
    struct ast_node * nn = new_ast_node(ast_assignment, value, NULL);
    nn->sybmol_reference.symbol = symbol;
    return (struct ast_node *) nn;
}

struct ast_node * ast_left(struct ast_node *l) {
    return l->left;
}

struct ast_node * ast_right(struct ast_node *l) {
    return l->right;
}

const char * ast_node_type_caption (ast_node_type_t t) {
    switch (t) {
        case ast_relational_g:
            return ">";
        case ast_relational_ge:
            return ">=";
        case ast_relational_l:
            return "<";
        case ast_relational_le:
            return "<=";
        case ast_arithmetic_minus:
            return "-";
        case ast_arithmetic_plus:
            return "+";
        case ast_arithmetic_div:
            return "/";
        case ast_arithmetic_mul:
            return "*";
        case ast_arithmetic_unary_min:
            return "-";
        case ast_logic_unary_not:
            return "!";
        case ast_logic_or:
            return "||";
        case ast_logic_and:
            return "&&";
        case ast_equality_equal:
            return "==";
        case ast_equality_not_equal:
            return "!=";
        case ast_function:
            return "f()";
        case ast_symbol_ref:
            return "S";
        case ast_assignment:
            return "=";
        case ast_number_int:
            return "int";
        case ast_number_float:
            return "float";
        case ast_bool_const:
            return "bool";
        case ast_str_const:
            return "str";
        case ast_list:
            return ",";

        default:
            return "!UNKNOWN!";
    }
}

const char * ast_node_full_caption(struct ast_node *n) {
    static char rv [255];

    switch (n->node_type) {
        case ast_relational_g:
        case ast_relational_ge:
        case ast_relational_l:
        case ast_relational_le:
        case ast_arithmetic_minus:
        case ast_arithmetic_plus:
        case ast_arithmetic_div:
        case ast_arithmetic_mul:
        case ast_arithmetic_unary_min:
        case ast_logic_unary_not:
        case ast_logic_or:
        case ast_logic_and:
        case ast_equality_equal:
        case ast_equality_not_equal:
        case ast_list:
        default:
            return ast_node_type_caption(n->node_type);

        case ast_statement:
            sprintf(rv, "Stmnt of");
            break;

        case ast_assignment:
            sprintf(rv, "%s = ...", n->sybmol_reference.symbol->name);
            break;

        case ast_function:
            sprintf(rv, "%s(%s)", n->sybmol_reference.symbol->name, n->left != NULL ? "..." : "");
            break;

        case ast_symbol_ref:
            sprintf(rv, "%s", n->sybmol_reference.symbol->name);
            break;

        case ast_number_int:
            sprintf(rv, "%ld", n->number_int.value);
            break;

        case ast_number_float:
            sprintf(rv, "%.3f", n->number_float.value);
            break;

        case ast_bool_const:
            sprintf(rv, "%s", n->bool_const.value ? "true" : "false");
            break;

        case ast_str_const:
            sprintf(rv, "%s", n->str_const.value);
            break;
    }
    return rv;
}

ast_node_class_t ast_node_class(struct ast_node *n) {

    switch (n->node_type) {
        case ast_relational_g:
        case ast_relational_ge:
        case ast_relational_l:
        case ast_relational_le:
        case ast_arithmetic_minus:
        case ast_arithmetic_plus:
        case ast_arithmetic_div:
        case ast_arithmetic_mul:
        case ast_logic_or:
        case ast_logic_and:
        case ast_equality_equal:
        case ast_equality_not_equal:
            return nc_binary;

        default:
            fprintf(stderr, "%s | %s no class defined for\n", __func__, ast_node_full_caption(n));
            return nc_binary;

        case ast_arithmetic_unary_min:
        case ast_logic_unary_not:
            return nc_unary;

        case ast_assignment:
        case ast_function:
        case ast_symbol_ref:
            return nc_reference;

        case ast_str_const:
        case ast_bool_const:
        case ast_number_int:
        case ast_number_float:
            return nc_const;

        case ast_statement:
        case ast_list:
            return nc_flow;
    }
}

static void print_leaf (FILE *stream, struct ast_node *node, int *num) {
    *num = *num + 1;
    node->dot_draw_num = *num;
    struct ast_node *l = ast_left(node);
    struct ast_node *r = ast_right(node);

    fprintf(stream, " n%d [label=\"%s\"];\n", node->dot_draw_num, ast_node_full_caption(node));
    if ( l != NULL) {
        print_leaf (stream, l, num);
        fprintf(stream, "\t n%d -> n%d \n", node->dot_draw_num, l->dot_draw_num);
    }
    if ( r != NULL) {
        print_leaf (stream, r, num);
        fprintf(stream, "\t n%d -> n%d \n", node->dot_draw_num, r->dot_draw_num);
    }
}

int ast_print_tree (FILE *stream, struct ast_node *n) {

    int node_num = 0;
    fprintf(stream, "digraph {\n");
    print_leaf(stream, n, &node_num);
    fprintf(stream, "}\n");

    return 0;
}

int print_ast_to_file (const char *filename, struct ast_node *n) {
    FILE *s = fopen (filename, "w");
    ast_print_tree(s, n);
    fclose(s);

    return 0;
}
