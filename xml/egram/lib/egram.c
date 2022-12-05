#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>


#include "egram.h"


token_t* tokenize(egram_parsing_context_t *cntx, const char *input, unsigned len, unsigned *processed);
tokenize_rv_t read_token(token_context_t *context, token_t *t, const char *input, unsigned len, unsigned *processed);
void tokenize_reset_context(token_context_t *tc);


void egram_reset_parser(egram_parsing_context_t *cntx) {
    tokenize_reset_context(&cntx->token_context);
}

static void print_indent(unsigned indent) {
    for (int i = 0; i < indent; i++) {
        printf("__");
    }
}

static void print_input_head(const char *input) {
    char c = 1;
    printf(" --- \"");
    for (int i = 0; i < 50 && c != 0; i++) {
        c = input[i];
        switch (c) {
            case 0:
                break;

            case '\"':
                printf("\\\"");
                break;

            case '\n':
            case '\r':
            case '\t':
                printf("\\n");
                break;

            default:
                printf("%c", c);
                break;
        }
    }
    printf("\"");

}

void print_token_info(token_t *t) {
    const char *tt;
    const char *tn = "";
    switch (t->type) {
        case TT_CONST: tt="TT_CONST"; tn = t->literal; break;
        case TT_ALPHANUM: tt="TT_ALPHANUM"; break;
        case TT_WHITESPACE: tt="TT_WHITESPACE"; break;
        case TT_CUSTOM: tt="TT_CUSTOM"; break;
        case TT_ANY_BUT: tt="TT_ANY_BUT"; tn = t->literal; break;
        default: tt="UNDEFINED"; break;
    }
    printf("%s \"%s\"", tt, tn);
}

void red () {
    printf("\033[1;31m");
}

void green () {
    printf("\033[0;32m");
}

void yellow() {
    printf("\033[1;33m");
}

void reset () {
    printf("\033[0m");
}

void print_match_or_miss(rule_rv_t status) {
    if (status == r_match) {
//        green();
        printf("MATCH");
//        reset();
    } else {
//        yellow();
        printf("MISS");
//        reset();
    }
}

static void dbg_msg(gsymbol_t *symbol, const char *curr_input, rule_rv_t status, int note_entrance, unsigned indent) {

    print_indent(indent);

    if (!note_entrance) {
        print_match_or_miss(status);
    } else {
        printf("Entering ");
    }

    if (symbol->type == GST_NONTERM) {
        printf("Non-term symbol \"%s\" to \"", symbol->name);
    } else {
        printf("Term symbol %s of type ", symbol->name);
        print_token_info(symbol);
    }

    print_input_head(curr_input);

    printf("...\"\n");
}

static rule_rv_t process_line(egram_parsing_context_t *cntx, gsymbol_t *upper_rule, const char *input, unsigned len, unsigned *processed, unsigned depth) {
    unsigned parsed_bytes;
    const char *initial_input = input;

    unsigned many_symb_matches = 0;

    unsigned i = 0;
#   define EAT_TERM() symb++; i++
    gsymbol_t *symb = upper_rule;
    rule_rv_t rrv = r_more;

    while ((len > 0) && !IS_END(*symb)) {
        parsed_bytes = 0;
        switch (symb->type) {
            case GST_TERM:
                ;
                print_indent(depth);
                printf("% 2d Lookup token: ", i);
                print_token_info(symb->t);
                print_input_head(input);
                printf("\n");
//                token_t *curr_token = tokenize(cntx, input, len, &parsed_bytes);
//                if (curr_token == symb->t) {
//                    rrv = r_match;
//                } else {
//                    rrv = r_miss;
//                }
                tokenize_reset_context(&cntx->token_context);
                tokenize_rv_t trv = read_token(&cntx->token_context, symb->t, input, len, &parsed_bytes);
                rrv = trv == t_match ? r_match : r_miss;

                print_indent(depth);
                printf("% 2d ", i);
                print_match_or_miss(rrv);
                printf("\n");

                break;

            case GST_NONTERM:
                ;
                unsigned rn;
                for (rn = 0; symb->elems[rn] != NULL; rn++);

                for (int j = 0; symb->elems[j] != NULL; j++) {
                    print_indent(depth);
                    printf("% 2d %02d/%02d of \"%s\": ", i, j+1, rn, symb->name);
                    print_input_head(input);
                    printf("\n");
                    rrv = process_line(cntx, symb->elems[j], input, len, &parsed_bytes, depth + 1);
                    print_indent(depth);
                    printf("% 2d %02d/%02d ", i, j+1, rn);
                    print_match_or_miss(rrv);
                    printf(" \"%s\" \n", symb->name);
                    if (rrv == r_match) {
                        break;
                        // TODO hanged rule set (stack?)
                        // TODO handle more
                    }
                }
                break;

            default:
                return r_err;
        }
//        dbg_msg(symb, input, rrv, 0, depth);

        if (rrv == r_match) {
            if (symb->h != NULL) {
                symb->h(cntx->user_data, cntx->token_context.value, cntx->token_context.offset);
            }
            if (!symb->many) {
                EAT_TERM();
                print_indent(depth); printf("% 2d SHIFT\n", i);
            } else {
                many_symb_matches++;
            }
        } else if (symb->optional) {
            EAT_TERM();
            print_indent(depth); printf("% 2d SHIFT OPTIONAL\n", i);
            continue; // start over for the next token
        } else {
            if (!symb->many) {
                many_symb_matches = 0;
            }
            break;
        }

        input += parsed_bytes;
        len -= parsed_bytes;
    }

    if (many_symb_matches > 0) {
        rrv = r_match;
    }

    if (rrv == r_match) {
        if (IS_END(*symb)) {
            if (symb->h != NULL) {
                symb->h(cntx->user_data, NULL, 0);
            }
        }
        *processed = input - initial_input;
        print_indent(depth); printf("% 2d LINE MATCH\n", i);
    } else {
        print_indent(depth); printf("% 2d LINE DEADEND\n", i);
    }

    return rrv;
}

rule_rv_t egram_process(egram_parsing_context_t *cntx, gsymbol_t  *symbol, const char *input, unsigned len, unsigned *processed) {
    return process_line(cntx, symbol, input, len, processed, 0);
}


