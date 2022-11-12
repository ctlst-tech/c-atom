#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "instr.h"
#include "ast.h"
#include "eint_types.h"
#include "symbol.h"
#include "eint.h"


// printf("rslt=%.2f l=%.2f r=%.2f", (*res), (*l_op), (*r_op));
#define OPERATION_BINARY(__o) {(*res) = (*l_op) __o (*r_op);}
#define OPERATION_UNARY(__o) {(*res) = __o (*l_op);}

#define ARITHM_BINARY_HANDLERS(__pref,__op) \
void __pref##_iii(const int *l_op, const int *r_op, int *res) { OPERATION_BINARY(__op); } \
void __pref##_fff(const double *l_op, const double *r_op, double *res) { OPERATION_BINARY(__op); } \
void __pref##_iff(const int *l_op, const double *r_op, double *res) { OPERATION_BINARY(__op); } \
void __pref##_fif(const double *l_op, const int *r_op, double *res) { OPERATION_BINARY(__op); }

#define COMPARISON_HANDLERS(__pref,__op) \
void __pref##_iib(const int *l_op, const int *r_op, boolval_t *res) { OPERATION_BINARY(__op); } \
void __pref##_ffb(const double *l_op, const double *r_op, boolval_t *res) { OPERATION_BINARY(__op); } \
void __pref##_ifb(const int *l_op, const double *r_op, boolval_t *res) { OPERATION_BINARY(__op); } \
void __pref##_fib(const double *l_op, const int *r_op, boolval_t *res) { OPERATION_BINARY(__op); }

void not_bb(const boolval_t *l_op, const int *no_val, boolval_t *res) { OPERATION_UNARY(!); }

void umin_ii(const int *l_op, const int *no_val, int *res) { OPERATION_UNARY(-); }
void umin_ff(const double *l_op, const double *no_val, double *res) { OPERATION_UNARY(-); }

void or_bbb(const boolval_t *l_op, const boolval_t *r_op, boolval_t *res) { OPERATION_BINARY(||); }
void and_bbb(const boolval_t *l_op, const boolval_t *r_op, boolval_t *res) { OPERATION_BINARY(&&); }

ARITHM_BINARY_HANDLERS(add, +)
ARITHM_BINARY_HANDLERS(sub, -)
ARITHM_BINARY_HANDLERS(mul, *)
ARITHM_BINARY_HANDLERS(div, /)

COMPARISON_HANDLERS(g, >)
COMPARISON_HANDLERS(l, <)
COMPARISON_HANDLERS(ge, >=)
COMPARISON_HANDLERS(le, <=)
COMPARISON_HANDLERS(eq, ==)
COMPARISON_HANDLERS(ne, !=)

void ne_bbb(const boolval_t *l_op, const boolval_t *r_op, boolval_t *res) { OPERATION_BINARY(!=); }
void eq_bbb(const boolval_t *l_op, const boolval_t *r_op, boolval_t *res) { OPERATION_BINARY(==); }

#define CMP_STR(s1,s2,__op,br) *br = (s1 != NULL && s2 !=NULL) && ((strcmp(s1,s2) __op 0));
void ne_ssb(const char *l_op, const char *r_op, boolval_t *res) { CMP_STR(l_op, r_op, !=, res); }
void eq_ssb(const char *l_op, const char *r_op, boolval_t *res) { CMP_STR(l_op, r_op, ==, res); }

#define OPERATION_ASSIGN() {*res = *l_op;}

void assign_bb(const boolval_t *l_op, const int *no_val, boolval_t *res) { OPERATION_ASSIGN(); }
void assign_fi(const int *l_op, const int *no_val, double *res) { OPERATION_ASSIGN(); }
void assign_ii(const int *l_op, const int *no_val, int *res) { OPERATION_ASSIGN(); }
void assign_ff(const double *l_op, const int *no_val, double *res) { OPERATION_ASSIGN(); }
void assign_if(const double *l_op, const int *no_val, int *res) { OPERATION_ASSIGN(); }


void func_handler(symbol_func_handler_t handler, symbol_func_args_list_t *al, result_val_t *res) {
    handler(al, res);
}

#define RULE_ARITHM(__pref) \
{ .tl = nr_int, .tr = nr_int, .tres = nr_int, .handler = CAST_HNDLR(__pref##_iii) }, \
{ .tl = nr_float, .tr = nr_float, .tres = nr_float, .handler = CAST_HNDLR(__pref##_fff) }, \
{ .tl = nr_int, .tr = nr_float, .tres = nr_float, .handler = CAST_HNDLR(__pref##_iff) }, \
{ .tl = nr_float, .tr = nr_int, .tres = nr_float, .handler = CAST_HNDLR(__pref##_fif) } \

#define RULE_COMPARISON(__pref) \
{ .tl = nr_int, .tr = nr_int, .tres = nr_bool, .handler = CAST_HNDLR(__pref##_iib) }, \
{ .tl = nr_float, .tr = nr_float, .tres = nr_bool, .handler = CAST_HNDLR(__pref##_ffb) }, \
{ .tl = nr_int, .tr = nr_float, .tres = nr_bool, .handler = CAST_HNDLR(__pref##_ifb) }, \
{ .tl = nr_float, .tr = nr_int, .tres = nr_bool, .handler = CAST_HNDLR(__pref##_fib) }\

#define RULE_BOOL_EQ(_pref) \
{ .tl = nr_bool, .tr = nr_bool, .tres = nr_bool, .handler = CAST_HNDLR(eq_bbb) }

#define RULE_BOOL_NEQ(_pref) \
{ .tl = nr_bool, .tr = nr_bool, .tres = nr_bool, .handler = CAST_HNDLR(ne_bbb) }

#define RULE_STR_EQ(_pref) \
{ .tl = nr_string, .tr = nr_string, .tres = nr_bool, .handler = CAST_HNDLR(eq_ssb) }

#define RULE_STR_NEQ(_pref) \
{ .tl = nr_string, .tr = nr_string, .tres = nr_bool, .handler = CAST_HNDLR(ne_ssb) }

#define RULE_UMIN() \
{ .tl = nr_int, .tr = nr_na, .tres = nr_int, .handler = CAST_HNDLR(umin_ii) }, \
{ .tl = nr_float, .tr = nr_na, .tres = nr_float, .handler = CAST_HNDLR(umin_ff) }

#define RULE_NOT() \
{ .tl = nr_bool, .tr = nr_na, .tres = nr_bool, .handler = CAST_HNDLR(not_bb) }

#define RULE_OR() \
{ .tl = nr_bool, .tr = nr_bool, .tres = nr_bool, .handler = CAST_HNDLR(or_bbb) }

#define RULE_AND() \
{ .tl = nr_bool, .tr = nr_bool, .tres = nr_bool, .handler = CAST_HNDLR(and_bbb) }

#define RULE_ASSIGN() \
{ .tl = nr_bool, .tr = nr_bool, .tres = nr_bool, .handler = CAST_HNDLR(assign_bb) }, \
{ .tl = nr_float, .tr = nr_int, .tres = nr_float, .handler = CAST_HNDLR(assign_fi) }, \
{ .tl = nr_int, .tr = nr_int, .tres = nr_int, .handler = CAST_HNDLR(assign_ii) }, \
{ .tl = nr_float, .tr = nr_float, .tres = nr_float, .handler = CAST_HNDLR(assign_ff) }, \
{ .tl = nr_int, .tr = nr_float, .tres = nr_int, .handler = CAST_HNDLR(assign_if) }

#define CAST_HNDLR(__h) ((instr_handler_t)(__h))

static const struct node_rules {
    ast_node_type_t   operation_type;
    const struct exact_rule{
        result_type_t tl; // type left
        result_type_t tr; // type right
        result_type_t tres; // type result
        instr_handler_t   handler;
    } rules[10];
} cast_rules_table [20]= {
        { .operation_type = ast_arithmetic_plus, .rules = { RULE_ARITHM(add) } },
        { .operation_type = ast_arithmetic_minus, .rules = { RULE_ARITHM(sub) } },
        { .operation_type = ast_arithmetic_mul, .rules = { RULE_ARITHM(mul) } },
        { .operation_type = ast_arithmetic_div, .rules = { RULE_ARITHM(div) } },

        { .operation_type = ast_relational_g, .rules = { RULE_COMPARISON(g) } },
        { .operation_type = ast_relational_ge, .rules = { RULE_COMPARISON(ge) } },
        { .operation_type = ast_relational_l, .rules = { RULE_COMPARISON(l) } },
        { .operation_type = ast_relational_le, .rules = { RULE_COMPARISON(le) } },
        { .operation_type = ast_equality_equal, .rules = { RULE_COMPARISON(eq), RULE_BOOL_EQ(), RULE_STR_EQ() } },
        { .operation_type = ast_equality_not_equal, .rules = { RULE_COMPARISON(ne), RULE_BOOL_NEQ(), RULE_STR_NEQ() } },

        {.operation_type = ast_arithmetic_unary_min, .rules = {RULE_UMIN()} },
        {.operation_type = ast_logic_unary_not, .rules = {RULE_NOT()} },

        {.operation_type = ast_logic_and, .rules = {RULE_AND()} },
        {.operation_type = ast_logic_or, .rules = {RULE_OR()} },
        {.operation_type = ast_assignment, .rules = {RULE_ASSIGN()} },

};



static const struct node_rules * find_rule_subtable(ast_node_type_t node_type) {
    int i;

    for (i = 0; cast_rules_table[i].operation_type != ast_void; i++) {
        if (cast_rules_table[i].operation_type == node_type) {
            return &cast_rules_table[i];
        }
    }

    return NULL;
}
static const struct exact_rule * find_exact_rule(const struct node_rules *rules_subtrable, result_type_t lr, result_type_t rr) {
    int i;

    for (i = 0; rules_subtrable->rules[i].handler != NULL; i++) {
        if ((rules_subtrable->rules[i].tl == lr) &&
                (rules_subtrable->rules[i].tr == rr)) {

            return &rules_subtrable->rules[i];
        }
    }

    return NULL;
}


result_reg_t *allocate_register(instr_queue_t *q, result_type_t rt) {

    if (q->reg_num >= EINT_MAX_REGS) {
        printf("Allowed instructions exceeded");
        return NULL;
    }

    result_reg_t *rv = &q->registers[q->reg_num++];
    rv->type = rt;

    return rv;
}

int bind_reg(result_reg_t *reslt, struct ast_node *n) {
    n->res_reg = reslt;
    return 0;
}

void * ref_result(result_reg_t *reslt) {
    switch (reslt->type) {
        case nr_float:      return &reslt->val.f;
        case nr_int:        return &reslt->val.i;
        case nr_bool:       return &reslt->val.b;
        case nr_string:     return (void*) reslt->val.s;
        default:            return NULL;
    }
}

static instr_t *instr_add_basics (instr_queue_t *q, instr_handler_t h, result_reg_t *reslt, struct ast_node *n) {
    instr_t *new;

    if (q->istr_num >= EINT_MAX_INSTR) {
        printf("Allowed instructions exceeded");
        return NULL;
    }

    new = &q->instructions[q->istr_num];

    new->res = ref_result(reslt);

    new->res_reg_ref = reslt;
    new->handler = h;
    new->node = n;
    //n->execution_order = q->istr_num;
    q->istr_num++;

    return new;
}

int instr_add_usual(instr_queue_t *q, instr_handler_t h, result_reg_t *reslt, struct ast_node *n) {
    instr_t *new;

    new = instr_add_basics(q, h, reslt, n);
    if (new == NULL) {
        return -1;
    }

    if (n->left != NULL) {
        new->l_operand = ref_result(n->left->res_reg);
    }

    if (n->right != NULL) {
        new->r_operand = ref_result(n->right->res_reg);
    }

    return 0;
}

int instr_add_func(instr_queue_t *q, symbol_func_handler_t h, symbol_func_args_list_t *al, result_reg_t *reslt, struct ast_node *n) {
    instr_t *new;

    new = instr_add_basics(q, CAST_HNDLR(func_handler), reslt, n);
    if (new == NULL) {
        return -1;
    }

    new->l_operand = h;
    new->r_operand = al;

    return 0;
}

int instr_arg_list_pass_data_handle(symbol_func_args_list_t *al, result_reg_t *hr) {

    if (al->args_num > 0) {
        return 1;
    }

    al->args[0] = hr;
    al->args_num++;

    return 0;
}

int instr_collect_args_list(const char *func_name, struct ast_node *n, symbol_func_args_list_t *al) {
    //struct ast_node *n;
    int rv;

    // TODO avoid recursion

    if (n->node_type == ast_list) {
        rv = instr_collect_args_list(func_name, n->left, al);
        if (rv) return rv;
        if (n->right != NULL) {
            rv = instr_collect_args_list(func_name, n->right, al);
            if (rv) return rv;
        }

    } else {
        if (al->args_num >= SYMBOL_FUNC_MAX_ARGS) {
            printf("too many args for %s\n", func_name);
            return -1;
        }
        al->args[al->args_num++] = n->res_reg;
    }

    return 0;
}

void *my_alloc(ssize_t s);

static int eint_instance_add_symb_state_handler(eint_instance_t *ei, symbol_state_handler_t *sh, void *state_ref) {
    symbol_state_handler_call_item_t *new;

    new = calloc(1, sizeof(*new));
    if (new == NULL) {
        return 1;
    }

    new->sh = sh;
    new->state_ref = state_ref;

    if (ei->shc_list == NULL) {
        ei->shc_list = new;
    } else {
        symbol_state_handler_call_item_t *n;
        for (n = ei->shc_list; n->next != NULL; n = n->next);
        n->next = new;
    }

    return 0;
}

#define compile_err_msg(txt,...) fprintf(stderr, "eint compilation | " txt "\n", ##__VA_ARGS__)


int instr_compile(struct ast_node *n, eint_instance_t *ei) {

    instr_queue_t *q = &ei->instr_queue;
    //printf("%d %s\n", q->istr_num, ast_node_full_caption(n));

    int rv = -1;
    result_reg_t *r;

    const struct node_rules *node_rules = find_rule_subtable(n->node_type);

    switch (ast_node_class(n)) {
        case nc_unary:
        case nc_binary:
            if (node_rules == NULL) {
                compile_err_msg("No rules group for %s", ast_node_full_caption(n));
                return -1;
            }
            result_type_t lr, rr;
            lr = n->left->res_reg->type;
            rr = n->right != NULL ? n->right->res_reg->type : nr_na;

            const struct exact_rule *rule = find_exact_rule(node_rules, lr, rr);

            if (rule == NULL) {
                compile_err_msg("No rule for \"%s\" op with %s & %s args", ast_node_full_caption(n),
                                eint_print_type_result(lr),
                                eint_print_type_result(rr));
                return -1;
            }

            r = allocate_register(q, rule->tres);
            rv = instr_add_usual(q, rule->handler, r, n);
            bind_reg(r, n);

            if (rv) {
                compile_err_msg("No valid rule for %s", ast_node_full_caption(n));
                return -1;
            }
            break;

        case nc_const:
            switch (n->node_type) {
                case ast_number_float:
                    //TODO register allocation check in every case
                    r = allocate_register(q, nr_float);
                    r->val.f = n->number_float.value;
                    bind_reg(r, n);
                    rv = 0;
                    break;

                case ast_number_int:
                    r = allocate_register(q, nr_int);
                    r->val.i = n->number_int.value;
                    bind_reg(r, n);
                    rv = 0;
                    break;

                case ast_bool_const:
                    r = allocate_register(q, nr_bool);
                    r->val.b = n->bool_const.value;
                    bind_reg(r, n);
                    rv = 0;
                    break;

                case ast_str_const:
                    r = allocate_register(q, nr_string);
                    r->val.s = (char *) n->str_const.value; // TODO reconsider using of direct reference
                    bind_reg(r, n);
                    rv = 0;
                    break;

                default:
                    compile_err_msg("No applied rule for const %s", ast_node_full_caption(n));
                    break;
            }
            break;

        case nc_flow:
            // TODO no work to do?
            // at least for comma
            rv = 0;
            break;

        case nc_reference:
            switch (n->sybmol_reference.symbol->type) {
                case symt_noref:
                    compile_err_msg("Symbol %s has no reference", n->sybmol_reference.symbol->name);
                    break;

                case symt_var:
                    if (n->node_type == ast_function) {
                        compile_err_msg("Symbol %s addressed as function, while it is not", n->sybmol_reference.symbol->name);
                        break;
                    }

                    bind_reg(&n->sybmol_reference.symbol->i.var, n);

                    if (n->node_type == ast_assignment) {
                        lr = n->sybmol_reference.symbol->i.var.type;
                        rr = n->left->res_reg->type;

                        rule = find_exact_rule(node_rules, lr, rr);
                        if (rule == NULL) {
                            compile_err_msg("No rule for assignment with %d %d args", lr, rr);
                            return -1;
                        }

                        rv = instr_add_usual(q, rule->handler, &n->sybmol_reference.symbol->i.var, n);
                    } else {
                        rv = 0;
                    }
                    break;

                case symt_func:
                    if (n->node_type != ast_function) {
                        compile_err_msg("Symbol %s not called as a function", n->sybmol_reference.symbol->name);
                        break;
                    }

                    // TODO wrong. must be attached somewhere in a datamodel. It hangs nowhere
                    symbol_func_args_list_t *al = my_alloc(sizeof (*al));

                    int expected_args_discount = 0;

                    if (n->sybmol_reference.symbol->i.func.state_handler != NULL) {
                        symbol_state_handler_t *sh = n->sybmol_reference.symbol->i.func.state_handler;
                        void *symbol_state_ref;
                        if (sh->state_constructor != NULL) {
                            rv = sh->state_constructor(sh->dhandle, &symbol_state_ref);
                            if (rv) {
                                break;
                            }
                        } else {
                            symbol_state_ref = sh->dhandle;
                        }

                        result_reg_t *dh = allocate_register(q, nr_ptr);
                        dh->val.p = symbol_state_ref;

                        rv = instr_arg_list_pass_data_handle(al, dh);
                        if (rv) {
                            break;
                        }
                        rv = eint_instance_add_symb_state_handler(ei, sh, symbol_state_ref);
                        if (rv) {
                            break;
                        }
                        expected_args_discount = 1;
                    }

                    if (n->left != NULL) {
                        rv = instr_collect_args_list(n->sybmol_reference.symbol->name, n->left, al);
                        if (rv) {
                            break;
                        }
                    }

                    //symbol_func_print_args(al);
                    int expected_args = al->args_num - expected_args_discount;
                    if (expected_args > n->sybmol_reference.symbol->i.func.expected_args_num) {
                        // TODO mark err
                        compile_err_msg("%s function expects %d arguments but got %d",
                                n->sybmol_reference.symbol->name,
                                n->sybmol_reference.symbol->i.func.expected_args_num,
                                expected_args);

                        rv = 1;
                        break;
                    }

                    r = allocate_register(q, n->sybmol_reference.symbol->i.func.ret_val_type);
                    rv = instr_add_func(q, n->sybmol_reference.symbol->i.func.handler, al, r, n);

                    bind_reg(r, n);
                    break;
            }

            break;
    }

    return rv;
}

int _instr_compile_ast(struct ast_node *root, eint_instance_t *ei) {
//int _instr_compile_ast(struct ast_node *root, instr_queue_t *q) {
    //TODO remove recursion

    int rv;

    if (root->left != NULL ) {
        rv = _instr_compile_ast(root->left, ei);
        if (rv) return rv;
    }

    if (root->right != NULL ) {
        rv = _instr_compile_ast(root->right, ei);
        if (rv) return rv;
    }

    return instr_compile(root, ei);
}

int instr_compile_ast(eint_instance_t *ei) {
    int rv;

    if (ei == NULL) {
        return 0; // TODO must be more specific return code
    }

    instr_queue_t *q = &ei->instr_queue;

    memset(q, 0, sizeof (*q));
    rv = _instr_compile_ast(ei->ast_root, ei);
    if (rv) {
        return rv;
    }

    if (q->istr_num > 0) {
        q->res_reg_ref = q->instructions[q->istr_num-1].res_reg_ref;
    } else {
        // root is const // TODO check this
        q->res_reg_ref = ei->ast_root->res_reg;
    }

    q->compiled = TRUE; // TODO makes no sense with memset above

    return 0;
}

result_reg_t *instr_queue_exec(instr_queue_t *q) {
    //int i;

    /*for (i = 0; i < q->istr_num; i++) {
        q->instructions[i].handler(q->instructions[i].l_operand, q->instructions[i].r_operand, q->instructions[i].result );
    }*/
    /*instr_t *instr = q->instructions;
    for (i = 0; i < q->istr_num; i++) {
        q->instructions[i].handler(q->instructions[i].l_operand, q->instructions[i].r_operand, q->instructions[i].result );
    }*/

    instr_t *instr = q->instructions;
    instr_t *instr_end = &q->instructions[q->istr_num];
    //i = q->istr_num;

    while (instr < instr_end) {
        instr->handler(instr->l_operand, instr->r_operand, instr->res);
        instr++;
    }

    return q->res_reg_ref;
}

//
//int instr_compile_ast_non_recrusive(struct ast_node *root, instr_queue_t *q) {
//    //  not operational!!
//
//    memset(q, 0, sizeof (*q));
//
//#   define MAX_AST_DEPTH 40
//    struct ast_node* nodes_stack[MAX_AST_DEPTH];
//    int nodes_stack_top = 0;
//    //  throw overflow
//
//#   define ns_push(__n) {nodes_stack[nodes_stack_top++] = __n;}
//#   define ns_top() nodes_stack[nodes_stack_top-1]
//#   define ns_pop() nodes_stack[nodes_stack_top--]
//#   define ns_empty() (nodes_stack_top == 0)
//
//    struct ast_node *curr = root;
//
//    while (curr != NULL || !ns_empty())
//    {
//        // Reach the left most Node of the
//        //   curr Node
//        while (curr->left !=  NULL)
//        {
////             place pointer to a tree node on
////               the stack before traversing
////              the node's left subtree
//            ns_push(curr);
//            curr = curr->left;
//        }
//
////         Current must be NULL at this point
//        curr = ns_top();
//        ns_pop();
//
//        instr_compile(q, curr->left);
//
////         we have visited the node and its
////           left subtree.  Now, it's right
////           subtree's turn
//        curr = curr->right;
//
//
//    }
//    // end of while
//
//    return 0;
//}

