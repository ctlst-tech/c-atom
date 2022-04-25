
%{
#include <stdio.h>

#include "ast.h"
#include "parser.tab.h"
#include "symbol.h"

%}

%option case-sensitive
%option nodefault noinput nounput noyywrap
%option reentrant bison-bridge
%option header-file="lexer.yy.h"

HEX_INT "-"?"0"[xX][0-9A-Fa-f]+

%%

[ \t\n\r]	                ; // ignore all whitespace
[0-9]+\.[0-9]+ 	        {yylval->fval = atof(yytext); return T_FLOAT;}
[0-9]+		            {yylval->ival = atoi(yytext); return T_INT;}
{HEX_INT}	            {yylval->ival = strtol(yytext, NULL, 16); return T_INT;}
"true"                  {yylval->bval = TRUE; return T_BOOLCONST;}
"false"                 {yylval->bval = FALSE; return T_BOOLCONST;}
\".*\"	                {yylval->sval = strndup(&yytext[1], strlen(yytext) - 2); return T_STRCONST;}
","	                    {return T_COMMA;}
"+"	                    {return T_PLUS;}
"-"	                    {return T_MINUS;}
"/"	                    {return T_DIV;}
"*"	                    {return T_MUL;}
[()]	                {return yytext[0];}
"!"	                    {return T_LOGIC_NOT;}
"=="	                {return T_EQUALITY_EQ;}
"!="	                {return T_EQUALITY_NEQ;}
">"	                    {return T_RELATIONAL_G;}
"<"	                    {return T_RELATIONAL_L;}
">="	                {return T_RELATIONAL_GE;}
"<="	                {return T_RELATIONAL_LE;}
"&&"	                {return T_LOGIC_AND;}
"||"	                {return T_LOGIC_OR;}
[a-zA-Z_][a-zA-Z0-9_]*    {yylval->symbol = symbol_lookup (cntx, yytext); return T_SYMBOL; }
"="	                    {return T_ASSIGN;}
";"	                    {return T_SEMICOLON;}

%%

// "//".*                    {return T_COMMENT;}

// https://github.com/meyerd/flex-bison-example/blob/master/calc.l
// https://stackoverflow.com/questions/780676/string-input-to-flex-lexer
