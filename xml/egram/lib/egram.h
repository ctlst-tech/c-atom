#ifndef C_ATOM_EGRAM_H
#define C_ATOM_EGRAM_H


typedef enum {
    TT_CONST,
    TT_ALPHANUM,
    TT_WHITESPACE,
    TT_CUSTOM,
} token_type_t;

#define MAX_TOKEN_LEN 256

typedef struct {
    char value[MAX_TOKEN_LEN + 1];
    size_t offset;
    unsigned state;
} token_context_t;

typedef enum {
    t_miss = 0,
    t_match,
    t_more,
    t_overflow,
} tokenize_rv_t;

typedef tokenize_rv_t (*custom_token_handler_t)(token_context_t *pc, const char *input, unsigned len, unsigned *processed);

typedef struct token {
    token_type_t type;
    char *literal;
    unsigned literal_size;
    custom_token_handler_t custom_handler;
} token_t;

typedef enum {
    GST_TERM,
    GST_NONTERM,
    RT_END
} symbol_type_t;

typedef const char * (*rule_handler_t)(void *context, const char* value, unsigned len);

struct gsybmbol;

typedef struct gsybmbol {
    const char *name;
    rule_handler_t h;
    symbol_type_t type;
    uint8_t many:1;
    uint8_t optional:1;

    union {
        token_t *t;
        struct gsybmbol **elems;
    };
} gsymbol_t;


typedef struct {
    gsymbol_t *current_rule;
    void *user_data;
    token_context_t token_context;
    token_t *hanged_token;
    int token_process_error;
    token_t **token_list;
} egram_parsing_context_t;


#define DEFINE_CONSTANT_STR_TOKEN(lit__) {.type=TT_CONST, .literal=(lit__), .literal_size=(sizeof(lit__)-1)}
#define DEFINE_CUSTOM_TOKEN(h__) {.type=TT_CUSTOM, .literal=NULL, .literal_size=0, .custom_handler=h__ }


#define DEFINE_TERMINAL(name__, tok__, many__, optional__, h__) { \
    .name = (name__),                          \
    .t = (tok__),                              \
    .type = GST_TERM,                          \
    .many = (many__),                          \
    .optional = (optional__),                  \
    .h = (h__)                                 \
}

#define DEFINE_NONTERMINAL(name__, elems__, many__, optional__, h__) \
{ \
    .name = (name__),                          \
    .elems = (elems__),                        \
    .type = GST_NONTERM,                       \
    .many = (many__),                          \
    .optional = (optional__),                  \
    .h = (h__)                                 \
}

#define TOKEN(name__, t__) DEFINE_TERMINAL(name__, &(t__), 0, 0, NULL)
#define TOKEN___H(name__, t__, h__) DEFINE_TERMINAL(name__, &(t__), 0, 0, h__)
#define TOKEN__OH(name__, t__, h__) DEFINE_TERMINAL(name__, &(t__), 0, 1, h__)

#define NONTERM____(name__, elems__) DEFINE_NONTERMINAL(name__, elems__, 0, 0, NULL)
#define NONTERM_MO_(name__, elems__) DEFINE_NONTERMINAL(name__, elems__, 1, 1, NULL)

#define T_WHITESPACE_MO DEFINE_TERMINAL("whitespace", &whitespace, 1, 1, NULL)

#define END {.type=RT_END}
#define END_WH(h__) {.h =(h__), .type=RT_END}
#define IS_END(r__) ((r__).type==RT_END)

typedef enum {
    r_more,
    r_err,
    r_miss,
    r_match,
    r_continue
} rule_rv_t;

void egram_reset_parser(egram_parsing_context_t *cntx);
rule_rv_t egram_process(egram_parsing_context_t *cntx, gsymbol_t  *symbol, const char *input, unsigned len, unsigned *processed);

#endif //C_ATOM_EGRAM_H
