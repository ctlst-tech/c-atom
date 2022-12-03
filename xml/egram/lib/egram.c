#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>


#include "egram.h"


token_t* tokenize(egram_parsing_context_t *cntx, const char *input, unsigned len, unsigned *processed);
void tokenize_reset__context(token_context_t *tc);


void egram_reset_parser(egram_parsing_context_t *cntx) {
    tokenize_reset__context(&cntx->token_context);
}

static void dbg_msg(gsymbol_t *symbol, const char *curr_input, rule_rv_t status, int note_entrance, unsigned indent) {

    for (int i = 0; i < indent; i++) {
        printf("__");
    }

    if (!note_entrance) {
        const char *sts = status == r_match ? "MATCH" : "MISS ";
        printf("%s ", sts);
    } else {
        printf("Entering ");
    }

    if (symbol->type == GST_NONTERM) {
        printf("Non-term symbol \"%s\" to \"", symbol->name);
    } else {
        const char *tt;
        const char *tn = "";
        switch (symbol->t->type) {
            case TT_CONST: tt="TT_CONST"; tn = symbol->t->literal; break;
            case TT_ALPHANUM: tt="TT_ALPHANUM"; break;
            case TT_WHITESPACE: tt="TT_WHITESPACE"; break;
            case TT_CUSTOM: tt="TT_CUSTOM"; break;
            case TT_ANY: tt="TT_ANY"; break;
            default: tt="UNDEFINED"; break;
        }
        printf("Term symbol %s of type %s \"%s\" lookup to \"", symbol->name, tt, tn);
    }
    char c = 1;
    for (int i = 0; i < 50 && c != 0; i++) {
        c = curr_input[i];
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
    printf("...\"\n");
}

static rule_rv_t process_line(egram_parsing_context_t *cntx, gsymbol_t *upper_rule, const char *input, unsigned len, unsigned *processed, unsigned depth) {
    unsigned parsed_bytes;
    const char *initial_input = input;

    unsigned matches = 0;

#   define EAT_TERM() symb++

    gsymbol_t *symb = upper_rule;
    rule_rv_t rrv = r_more;

    dbg_msg(symb, input, rrv, 1, depth);


    while ((len > 0) && !IS_END(*symb)) {
        parsed_bytes = 0;
        switch (symb->type) {
            case GST_TERM:;
                token_t *curr_token = tokenize(cntx, input, len, &parsed_bytes);
                if (curr_token == symb->t) {
                    rrv = r_match;
                } else {
                    rrv = r_miss;
                }
                break;

            case GST_NONTERM:
                for (int i = 0; symb->elems[i] != NULL; i++) {
                    rrv = process_line(cntx, symb->elems[i], input, len, &parsed_bytes, depth + 1);
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
        dbg_msg(symb, input, rrv, 0, depth);

        if (rrv == r_match) {
            if (symb->h != NULL) {
                symb->h(cntx->user_data, cntx->token_context.value, cntx->token_context.offset);
            }
            if (!symb->many) {
                EAT_TERM();
            }
            matches++;
        } else if (symb->optional) {
            EAT_TERM();
            continue; // start over for the next token
        } else {
            break;
        }

        input += parsed_bytes;
        len -= parsed_bytes;
    }

    if (matches > 0) {
        rrv = r_match;
    }

    if (rrv == r_match) {
        if (IS_END(*symb)) {
            if (symb->h != NULL) {
                symb->h(cntx->user_data, NULL, 0);
            }
        }
        *processed = input - initial_input;
    }

    return rrv;
}

rule_rv_t egram_process(egram_parsing_context_t *cntx, gsymbol_t  *symbol, const char *input, unsigned len, unsigned *processed) {
    return process_line(cntx, symbol, input, len, processed, 0);
}