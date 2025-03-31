#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>


#include "egram.h"

//#define EGRAM_DEBUG 1
#undef EGRAM_DEBUG

token_t* tokenize(egram_parsing_context_t *cntx, const char *input, unsigned len, unsigned *processed);
tokenize_rv_t read_token(token_context_t *context, token_t *t, const char *input, unsigned len, unsigned *processed);
void tokenize_reset_context(token_context_t *tc);


void egram_reset_parser(egram_parsing_context_t *cntx) {
//    memset(cntx, 0, sizeof(*cntx));
    // TODO do that properly, all members
    tokenize_reset_context(&cntx->token_context);
}

#ifdef EGRAM_DEBUG

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

#endif

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
                #ifdef EGRAM_DEBUG
                print_indent(depth);
                printf("% 2d Lookup token: ", i);
                print_token_info(symb->t);
                print_input_head(input);
                printf("\n");
                #endif

                tokenize_reset_context(&cntx->token_context);
                tokenize_rv_t trv = read_token(&cntx->token_context, symb->tg.t, input, len, &parsed_bytes);
                rrv = trv == t_match ? r_match : r_miss;

                #ifdef EGRAM_DEBUG
                print_indent(depth);
                printf("% 2d ", i);
                print_match_or_miss(rrv);
                printf("\n");
                #endif

                break;

            case GST_NONTERM:
                ;
                #ifdef EGRAM_DEBUG
                unsigned rn;
                for (rn = 0; symb->elems[rn] != NULL; rn++);
                #endif

                for (int j = 0; symb->tg.elems[j] != NULL; j++) {

                    #ifdef EGRAM_DEBUG
                    print_indent(depth);
                    printf("% 2d %02d/%02d of \"%s\": ", i, j+1, rn, symb->name);
                    print_input_head(input);
                    printf("\n");
                    #endif

                    rrv = process_line(cntx, symb->tg.elems[j], input, len, &parsed_bytes, depth + 1);

                    #ifdef EGRAM_DEBUG
                    print_indent(depth);
                    printf("% 2d %02d/%02d ", i, j+1, rn);
                    print_match_or_miss(rrv);
                    printf(" \"%s\" \n", symb->name);
                    #endif

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

        if (rrv == r_match) {
            if (symb->h != NULL) {
                symb->h(cntx->user_data, cntx->token_context.value, cntx->token_context.offset);
            }
            if (!symb->many) {
                EAT_TERM();
                #ifdef EGRAM_DEBUG
                print_indent(depth); printf("% 2d SHIFT\n", i);
                #endif
            } else {
                many_symb_matches++;
            }
        } else if (symb->optional) {
            EAT_TERM();
            #ifdef EGRAM_DEBUG
            print_indent(depth); printf("% 2d SHIFT OPTIONAL\n", i);
            #endif
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
        #ifdef EGRAM_DEBUG
        print_indent(depth); printf("% 2d LINE MATCH\n", i);
        #endif
    } else {
        #ifdef EGRAM_DEBUG
        print_indent(depth); printf("% 2d LINE DEADEND\n", i);
        #endif
    }

    return rrv;
}

rule_rv_t egram_process(egram_parsing_context_t *cntx, gsymbol_t  *symbol, const char *input, unsigned len, unsigned *processed) {
    return process_line(cntx, symbol, input, len, processed, 0);
}
