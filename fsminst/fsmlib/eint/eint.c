#include "eint.h"

#include "ast.h"
#include "symbol.h"
#include "instr.h"

#include "parser.tab.h"
#include "lexer.yy.h"

typedef struct {
    yyscan_t scanner;
} parser_lexer_flex_bison_t;

int flex_bison_init(parser_lexer_flex_bison_t *cfg) {

    memset(cfg, 0, sizeof(*cfg));

    int rv = yylex_init(&cfg->scanner);
    if ( rv == 1 ) {
        return 1;
    }

    return 0;
}

int flex_bison_destroy(parser_lexer_flex_bison_t *cfg) {
    yylex_destroy(cfg->scanner);
    return 0;
}

int flex_bison_parse(parser_lexer_flex_bison_t *cfg, symbol_context_t *context, struct ast_node **root, const char *str2parse) {
    YY_BUFFER_STATE bs = yy_scan_string(str2parse, cfg->scanner);
    yyparse(cfg->scanner, context, root);
    yy_delete_buffer(bs, cfg->scanner);

    // TODO errors
    return 0;
}


int eint_init_and_compile(eint_instance_t *ei, symbol_context_t *cntx, const char *str2parse) {

    memset(ei, 0, sizeof(*ei));

    ei->context_ref = cntx;

    parser_lexer_flex_bison_t lexer_parser_cfg;

    flex_bison_init(&lexer_parser_cfg);

    flex_bison_parse(&lexer_parser_cfg, ei->context_ref, &ei->ast_root, str2parse);
    flex_bison_destroy(&lexer_parser_cfg);

    return instr_compile_ast(ei);

}

result_reg_t *eint_exec(eint_instance_t *i) {
    return instr_queue_exec(&i->instr_queue);
}

const char *eint_print_result(result_reg_t *r) {
    static char rv [255];

    switch (r->type) {
        case nr_na:
            sprintf(rv, "N/A");
            break;
        case nr_float:
            sprintf(rv, "%.5f", r->val.f);
            break;
        case nr_int:
            sprintf(rv, "%d", r->val.i);
            break;
        case nr_string:
            sprintf(rv, "%s", r->val.s);
            break;
        case nr_bool:
            sprintf(rv, "%s", r->val.b ? "true" : "false");
            break;

        case nr_ptr:
            sprintf(rv, "0x%p", r->val.p);
            break;
    }

    return rv;
}

const char *eint_print_type_result(result_type_t t) {
    switch (t) {
        case nr_na: return "NA";
        case nr_float: return "FLOAT";
        case nr_int: return "INT";
        case nr_string: return "STRING";
        case nr_bool: return "BOOL";
        case nr_ptr: return "PTR";
    }

    return "!UNKNOWN!";
}

result_type_t eint_parse_type(const char *type) {
    if (!strcasecmp(type, "float")) return nr_float;
    else if (!strcasecmp(type, "int")) return nr_int;
    else if (!strcasecmp(type, "string")) return nr_string;
    else if (!strcasecmp(type, "bool")) return nr_bool;
    else if (!strcasecmp(type, "ptr")) return nr_ptr;
    else return nr_na;
}

double my_fabs(double v) {return v < 0 ? -v : v;}

int eint_res_equal(result_reg_t *l, result_reg_t *r) {
    if (l->type != r->type) {
        return 0;
    }

    switch (l->type) {
        case nr_na: return 0;
        case nr_float: return l->val.f == r->val.f;//my_fabs(l->val.f - r->val.f) < 0.0000001;
        case nr_int: return l->val.i == r->val.i;
        case nr_string: return !strcmp(l->val.s, r->val.s);
        case nr_bool: return l->val.b == r->val.b;
        case nr_ptr: return l->val.p == r->val.p;
    }

    return 0;
}

#define EXEC_EXPRESSION_NATIVELY(exp_res_type__,r__,expr,str) \
    if ((exp_res_type__) == nr_float) ((r__).val.f = (expr)); else \
    if ((exp_res_type__) == nr_int) ((r__).val.i = (expr)); else             \
    if ((exp_res_type__) == nr_bool) ((r__).val.b = (expr)); \
    (r__).type = exp_res_type__;
    //    if (((exp_res_type__) == nr_string) && str) ((r__).val.s = (expr));

static int print_ast(const char *expr, symbol_context_t *cntx) {
    eint_instance_t inst;
    eint_init_and_compile(&inst, cntx, expr);
    // TODO do not compile
    return print_ast_to_file("test.dot", inst.ast_root);
}

static void eint_print_all_vars_value_(struct ast_node *n) {
    if (!n) {
        return;
    }

    if ((ast_node_class(n) == nc_reference) && (n->sybmol_reference.symbol->type == symt_var)) {
        printf( "%s = %s ", n->sybmol_reference.symbol->name, eint_print_result(n->res_reg));
    }

    eint_print_all_vars_value_(n->left);
    eint_print_all_vars_value_(n->right);
}

void eint_print_all_vars(struct ast_node *n) {
    eint_print_all_vars_value_(n);
    printf("\n");
}


int do_test(const char *expr, symbol_context_t *cntx, result_reg_t *expected_result) {
    eint_instance_t inst;
    result_reg_t dummy;
    result_reg_t* r;
    int passed;

    char *err = "";
    if (eint_init_and_compile(&inst, cntx, expr)) {
        err = "compilation error";
        r = &dummy;
        passed = 0;
    } else {
        r = eint_exec(&inst);
        passed = eint_res_equal(r, expected_result);
    }

    printf("%6s %s Testing expr: \"%s\" (expr result = %s, type is %s) %s\n", passed ? "PASSED" : "FAILED", err, expr,
           eint_print_result(r), eint_print_type_result(r->type),
           expected_result->type != r->type ? "native result type differs from computation result, check params" : "");

    return passed;
}

void test_func_handler (symbol_func_args_list_t *al, result_val_t *rv) {
    printf("%s call", __func__);
    symbol_func_print_args(al);
    rv->b = TRUE;
};

#define INT_FUNC_TEST_VAL 123456

void test_int_func_handler (symbol_func_args_list_t *al, result_val_t *rv) {
    rv->i = INT_FUNC_TEST_VAL;
};

int tests() {
    int tests = 0, failures = 0;

    symbol_context_t context = {.symbols = NULL};
    result_reg_t expected_result;

    //char *str2parse = "c= (a+b) * 20 * 20.4";
    //char *str2parse = "c= func( (a+b) * 20 * 20.4, 0.0, test_val)";
    //char *str2parse = "a, b, c";
    //char *str2parse = "c=func()";
    //char *str2parse = "a+b";
    //char *str2parse = "-20.15 + 20.25 * 10 + 2048 * 128 + 257.9232134";


    //print_ast_to_file ("test_ast_tree.dot", root);


#   define CHECK_AND_STAT(v)                    {tests++; if(!(v)) {failures++;}}
#   define RUN_TEST(expr__,ret_type__)               \
        EXEC_EXPRESSION_NATIVELY(ret_type__,expected_result, expr__, 0); \
        CHECK_AND_STAT(do_test(#expr__, &context, &expected_result))

    RUN_TEST(1+2, nr_int);
    RUN_TEST(1+2+30000+2000, nr_int);
    RUN_TEST(0.1+2.213+30000.448+2000.1546, nr_float);
    RUN_TEST(1 > 2, nr_bool);
    RUN_TEST(1 >= 2, nr_bool);
    RUN_TEST(1 == 1, nr_bool);
    RUN_TEST(1.01 == 1.02, nr_bool);
    RUN_TEST(0xfe == 0xff, nr_bool);
    RUN_TEST(0xfe != 0xfe, nr_bool);
    RUN_TEST(0xFF != 0xfe, nr_bool);
    RUN_TEST(0xAb != 0xCd, nr_bool);
    RUN_TEST(1.01 != 1.02, nr_bool);
    RUN_TEST(1.01 != 1.01, nr_bool);
    RUN_TEST(1.01 < 1.02, nr_bool);
    RUN_TEST(-1 == 1-2, nr_bool);
    RUN_TEST(!(1 > 2) && (2 > 1), nr_bool);
    RUN_TEST(-20.15 + 20.25 * 10 + 2048 * 128 + 257.9232134, nr_float);

    result_reg_t er = {.val.b = FALSE, .type=nr_bool};
    CHECK_AND_STAT(do_test("true == false", &context, &er));

    er.val.b = TRUE; er.type = nr_bool;
    CHECK_AND_STAT(do_test("true", &context, &er));

#define symbol_update_double symbol_update_float
#define DECLARE_TEST_VAR(n__,v__,vte__,vtn__) vtn__ n__=v__; symbol_t *n__##sym = symbol_add_var(&context, #n__, vte__); symbol_update_w_str(n__##sym, #v__)

    DECLARE_TEST_VAR(a, 0.123, nr_float, double);
    DECLARE_TEST_VAR(b, 0.321, nr_float, double);
    DECLARE_TEST_VAR(c, 20, nr_int, int);
    DECLARE_TEST_VAR(d, 33.0, nr_float, float);
//    DECLARE_TEST_VAR(str, "test", nr_string, const char*);
    symbol_t *str_symb = symbol_add_var(&context, "sv", nr_string);
    symbol_update(str_symb, "lalala");

    er.val.b = TRUE; er.type = nr_bool;
    CHECK_AND_STAT(do_test("sv == \"lalala\"", &context, &er));


    RUN_TEST(a+b+c+d, nr_float);

    symbol_state_handler_t sh = {
        .dhandle = &context,
        .state_activate = NULL,
        .state_constructor = NULL
    };

    symbol_add_func(&context, "test_func", test_func_handler, &sh, 4, nr_bool);
    symbol_add_func(&context, "test_int_func", test_int_func_handler, &sh, 0, nr_int);

    //print_ast("-20.15 + 20.25 * 10 + 2048 * 128 + 257.9232134", &symbols_context);
    //print_ast("test_func(a,b+21,c,d+45)", &symbols_context);
    print_ast("test_func(a*2,b*4,c-456,d+c)", &context);

    er.val.b = TRUE;
    er.type = nr_bool;

    CHECK_AND_STAT(do_test("test_func(a,b,c,d)", &context, &er));
    //CHECK_AND_STAT(do_test("test_func(a*2,b*4,c-456,d+c)", &symbols_context, &er));

    er.val.f = 123.456; er.type = nr_float;
    CHECK_AND_STAT(do_test("d = 123.456;", &context, &er));
    printf("Assignment of d; d == %.4f\n", dsym->i.var.val.f);

    er.val.f = 90; er.type = nr_float;
    CHECK_AND_STAT(do_test("d = 90;", &context, &er));
    printf("Assignment of d; d == %.4f\n", dsym->i.var.val.f);

    er.val.i = 111; er.type = nr_int;
    CHECK_AND_STAT(do_test("c = 111.111;", &context, &er));
    printf("Assignment of c; c == %d\n", csym->i.var.val.i);

    er.val.i = INT_FUNC_TEST_VAL; er.type = nr_int;
    CHECK_AND_STAT(do_test("c = test_int_func();", &context, &er));
    printf("Assignment of c by func; c == %d\n", csym->i.var.val.i);

    printf("Tests performed: %d, failures: %d\n", tests, failures);

    return failures;
}

#include <time.h>

int test_performance(const char *str2exec) {
    eint_instance_t inst;
    symbol_context_t context = {NULL};

    printf("Testing performance for expression: \"%s\"\n", str2exec);

    eint_init_and_compile(&inst, &context, str2exec);

    for (int j = 0; j < 10; j++) {
        struct timespec t0, t1;

        result_reg_t *r;

        clock_gettime(CLOCK_MONOTONIC, &t0);
#   define DT(t1, t0) ( (t1).tv_sec - (t0).tv_sec + ((t1).tv_nsec - (t0).tv_nsec) * 1E-9 )
        for (int i = 0; i < 10000000; i++) {
            r = eint_exec(&inst);
            //r.f = (volatile) ((volatile)20.15 + (volatile)20.25 * (volatile)10 + (volatile)2048 * (volatile)128 + (volatile)257.9232134);
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);

        printf("% d. result = %s computation time = %f\n", j+1, eint_print_result(r), DT(t1, t0));
    }
    return 0;
}


int eint_test_all () {

    tests();

    //test_performance("-20.15 + 20.25 * 10 + 2048 * 128 + 257.9232134");

    return 0;
}
