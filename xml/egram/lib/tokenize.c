#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "egram.h"


static tokenize_rv_t read_token_const(token_context_t *pc, token_t *t, const char *input, unsigned len, unsigned *processed) {
    tokenize_rv_t rv = t_more;
    int loop = 1;
    unsigned i;

    for (i = 0; i < len; i++) {
        if (input[i] == t->literal[pc->offset]) {
            pc->offset++;
            if (pc->offset == t->literal_size) {
                rv = t_match;
                break;
            }
        } else {
            rv = t_miss;
            break;
        }
    }

    if (rv == t_match) {
        i++; // last byte
        *processed = i;
    }

    return rv;
}

static tokenize_rv_t read_token_whitespace(token_context_t *tc, const char *input, unsigned len, unsigned *processed) {
    if (isspace(input[0])) {
        *processed = 1;
        return t_match;
    } else {
        return t_miss;
    }
}

static tokenize_rv_t read_token_alphanum(token_context_t *tc, const char *input, unsigned len, unsigned *processed) {
    unsigned i = 0;
    tokenize_rv_t rv;
    int loop = -1;
    for(i = 0; i < len; i++) {
        if (isalnum(input[i]) || (input[i] == '_')) { // FIXME '_'
            tc->value[tc->offset] = input[i];
            tc->offset++;

            // TODO check offset range
            rv = t_more;
        } else if (tc->offset > 0) {
            rv = t_match;
            break;
        } else {
            rv = t_miss;
            break;
        }
    }

    if (rv == t_match) {
        *processed = i;
    }
    return rv;
}


static tokenize_rv_t read_token(token_context_t *context, token_t *t, const char *input, unsigned len, unsigned *processed) {
    switch (t->type) {
        case TT_CONST: return read_token_const(context, t, input, len, processed);
        case TT_ALPHANUM: return read_token_alphanum(context, input, len, processed);
        case TT_WHITESPACE: return read_token_whitespace(context, input, len, processed);
        case TT_CUSTOM: return t->custom_handler(context, input, len, processed);
//        case TT_ANY: read_token_any(context, t_next, input, len, processed);
    }
}


void tokenize_reset__context(token_context_t *tc) {
    tc->offset = 0;
    tc->state = 0;
}


token_t* tokenize(egram_parsing_context_t *cntx, const char *input, unsigned len, unsigned *processed) {
    tokenize_rv_t tr;
    token_t *t;
    int i = 0;
    unsigned used_bytes = 0;

    if (cntx->hanged_token != NULL) {
        tr = read_token(&cntx->token_context, cntx->hanged_token, input, len, processed);
    } else {
        for (i = 0; cntx->token_list[i] != NULL; i++) {
            tokenize_reset__context(&cntx->token_context);
            tr = read_token(&cntx->token_context, cntx->token_list[i], input, len, &used_bytes);
            if (tr != t_miss) {
                break;
            }
        }
    }

    t = cntx->token_list[i];

    switch (tr) {
        case t_more:
            if (cntx->hanged_token == NULL) {
                cntx->hanged_token = t;
            }
            return cntx->hanged_token;

        case t_match:
            *processed = used_bytes;
            cntx->hanged_token = NULL;
            return t;

        case t_miss:
            cntx->hanged_token = NULL;
            return NULL;

        default:
        case t_overflow:
            cntx->token_process_error = -1;
            return t;
    }
}
