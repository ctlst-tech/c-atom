%code top {
  #include <assert.h>
  #include <ctype.h>  /* isdigit. */
  #include <stdio.h>  /* printf. */
  #include <stdlib.h> /* abort. */
  #include <string.h> /* strcmp. */



  #include "parser.tab.h"
  #include "lexer.yy.h"
  #include "symbol.h"
  #include "ast.h"
  #include "eint_types.h"

}

%code provides
{

#define YY_DECL int yylex \
        (YYSTYPE * yylval_param, void *yyscanner, symbol_context_t *cntx)

void
yyerror (void *scanner, symbol_context_t *cntx, struct ast_node **r, char const *s);

YY_DECL;
}

%code requires {
    #include "eint_types.h"
    #include "symbol.h"
}

%define api.pure full

%lex-param {void *scanner} {symbol_context_t *cntx}
%parse-param {void *scanner} {symbol_context_t *cntx} {struct ast_node **r}

%define parse.error verbose
%define parse.trace

%union {
  struct ast_node * ast;

  double fval;
  int ival;
  boolval_t bval;
  char *sval;

  int operator;

  struct symbol * symbol;
}

%token T_EOF 0
%token <ival> T_INT
%token <fval> T_FLOAT
%token <bval> T_BOOLCONST
%token <symbol> T_SYMBOL
%token <sval> T_STRCONST

%right T_COMMENT
%left T_COMMA
%right T_ASSIGN
%left T_LOGIC_AND
%left T_LOGIC_OR
%left T_EQUALITY_EQ
%left T_EQUALITY_NEQ
%left T_RELATIONAL_G
%left T_RELATIONAL_GE
%left T_RELATIONAL_L
%left T_RELATIONAL_LE
%left T_PLUS T_MINUS
%left T_MUL T_DIV
%right T_LOGIC_NOT
%right T_UMINUS
%left '(' ')'
%left T_SEMICOLON
%left T_DQUOTE


%type <ast> exp exp_list assignment statement top

%%

top: T_EOF
  | exp T_EOF { *r = $$; }
  | exp_list T_EOF { *r = $$; }
  | statement T_EOF { *r = $$; }

statement:
   assignment T_SEMICOLON	{$$ = new_ast_statement_node ($1, NULL);}
  | assignment T_SEMICOLON statement		{$$ = new_ast_statement_node ($1, $3);}

assignment:
  T_SYMBOL T_ASSIGN exp       { $$ = new_ast_assignment_node ($1, $3); }

exp_list:
    exp
  | exp T_COMMA exp_list { $$ = new_ast_node (ast_list, $1, $3); }

exp:
    exp T_RELATIONAL_G exp    { $$ = new_ast_node (ast_relational_g, $1, $3); }
  | exp T_RELATIONAL_GE exp    { $$ = new_ast_node (ast_relational_ge, $1, $3); }
  | exp T_RELATIONAL_L exp    { $$ = new_ast_node (ast_relational_l, $1, $3); }
  | exp T_RELATIONAL_LE exp    { $$ = new_ast_node (ast_relational_le, $1, $3); }
  | exp T_EQUALITY_EQ exp      { $$ = new_ast_node (ast_equality_equal, $1, $3); }
  | exp T_EQUALITY_NEQ exp      { $$ = new_ast_node (ast_equality_not_equal, $1, $3); }
  | exp T_LOGIC_AND exp         { $$ = new_ast_node (ast_logic_and, $1, $3); }
  | exp T_LOGIC_OR exp         { $$ = new_ast_node (ast_logic_or, $1, $3); }
  | exp T_PLUS exp           { $$ = new_ast_node (ast_arithmetic_plus, $1, $3); }
  | exp T_MINUS exp           { $$ = new_ast_node (ast_arithmetic_minus, $1, $3);}
  | exp T_MUL exp           { $$ = new_ast_node (ast_arithmetic_mul, $1, $3); }
  | exp T_DIV exp           { $$ = new_ast_node (ast_arithmetic_div, $1, $3); }
  | '(' exp ')'           { $$ = $2; }
  | T_MINUS exp %prec T_UMINUS  { $$ = new_ast_node (ast_arithmetic_unary_min, $2, NULL); }
  | T_LOGIC_NOT exp %prec T_LOGIC_NOT  { $$ = new_ast_node (ast_logic_unary_not, $2, NULL); }
  | T_INT                { $$ = new_ast_number_int_node ($1); }
  | T_FLOAT                { $$ = new_ast_number_float_node ($1); }
  | T_BOOLCONST               { $$ = new_ast_bool_const_node ($1); }
  | T_STRCONST                { $$ = new_ast_str_const_node ($1); }
  | T_SYMBOL                  { $$ = new_ast_symbol_reference_node ($1); }
  | T_SYMBOL '(' ')'          { $$ = new_ast_function_node ($1, NULL); }
  | T_SYMBOL '(' exp_list ')' { $$ = new_ast_function_node ($1, $3); }

%%


void
yyerror (void *scanner, symbol_context_t *cntx, struct ast_node **r, char const *s)
{
  fprintf (stderr, "%s\n", s);
}
