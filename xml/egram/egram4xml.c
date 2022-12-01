#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lib/egram.h"

typedef struct {
    const char *name;
    const char *value;
} attr_t;

#define MAX_ATTRS 16
#define MAX_TAG_BUFFER 256

typedef struct {
    const char *tagname;
    unsigned attrs_num;
    attr_t attrs [MAX_ATTRS];
    char tag_buffer[MAX_TAG_BUFFER];
    unsigned tag_buffer_offset;
} xml_parse_context_t;

void *allocate_buff(xml_parse_context_t *c, size_t size) {
    if (c->tag_buffer_offset + size > MAX_TAG_BUFFER){
        return NULL;
    }
    void *origin = &c->tag_buffer[c->tag_buffer_offset];
    c->tag_buffer_offset += size;
    return origin;
}

const char *place_str(xml_parse_context_t *c, const char* s){
    unsigned l = strlen(s);
    char *b = allocate_buff(c, l+1);
    if (b == NULL) {
        return NULL;
    }

    memcpy(b, s, l);
    b[l] = 0;

    return b;
}


const char *value(void *context, const char* value) {
    printf("Value: %s\n", value);
    return value;
}

const char *set_tag_name(void *context, const char* value) {
    xml_parse_context_t *c = (xml_parse_context_t *) context;
    const char *s = place_str(c, value);
    // TODO handle error
    c->tagname = s;
    return NULL;
}

const char *set_attr_name(void *context, const char* value) {
    xml_parse_context_t *c = (xml_parse_context_t *) context;
    const char *s = place_str(c, value);
    // TODO handle error
    c->attrs[c->attrs_num].name = s;
    return NULL;
}

const char *set_attr_value(void *context, const char* value) {
    xml_parse_context_t *c = (xml_parse_context_t *) context;
    const char *s = place_str(c, value);
    // TODO handle error
    c->attrs[c->attrs_num].value = s;
    return NULL;
}

const char *accept_attr(void *context, const char* value) {
    xml_parse_context_t *c = (xml_parse_context_t *) context;
    c->attrs_num++;
    return NULL;
}

const char *do_tag_reset(void *context, const char* value) {
    xml_parse_context_t *c = (xml_parse_context_t *) context;
    c->attrs_num = 0;
    c->tag_buffer_offset = 0;
    return NULL;
}

const char *got_tag_open(void *context, const char* value) {
    xml_parse_context_t *c = (xml_parse_context_t *) context;
    printf("Got tag \"%s\" open with %d attrs:\n", c->tagname, c->attrs_num);
    for (unsigned i = 0; i < c->attrs_num; i++) {
        printf("    Attr: %s=\"%s\"\n", c->attrs[i].name, c->attrs[i].value);
    }
    return NULL;
}

const char *got_tag_close(void *context, const char* value) {
    xml_parse_context_t *c = (xml_parse_context_t *) context;
    printf("Got tag \"%s\" close\n", c->tagname);
    return NULL;
}

const char *got_tag_empty(void *context, const char* value) {
    got_tag_open(context, value);
    got_tag_close(context, value);
    return NULL;
}

tokenize_rv_t attr_val(token_context_t *pc, const char *input, unsigned len, unsigned *consumed) {
    tokenize_rv_t rv;
    unsigned i = 0;

    switch (pc->state) {
        case 0:
            if (input[i]=='\"') {
                pc->state = 1;
                i++;
                rv = t_more;
            } else {
                rv = t_miss;
                break;
            }
            // fall through
        case 1:
            for (; i < len; i++) {
                if (input[i] == '\"') {
                    rv = t_match;
                    i++;
                    *consumed = i;
                    break;
                } else {
                    pc->value[pc->offset++] = input[i];
                    if (pc->offset > MAX_TOKEN_LEN) {
                        rv = t_overflow;
                        break;
                    }
                }
            }
            break;
    }

    return rv;
}

token_t token_attr_value =        DEFINE_CUSTOM_TOKEN(attr_val);

token_t whitespace = {.type = TT_WHITESPACE};
token_t alphanum = {TT_ALPHANUM};

token_t token_attr_equal =        DEFINE_CONSTANT_STR_TOKEN("=");
token_t token_comment_begin =     DEFINE_CONSTANT_STR_TOKEN("<!--");
token_t token_tag_closing_begin = DEFINE_CONSTANT_STR_TOKEN("</");
token_t token_header_begin =      DEFINE_CONSTANT_STR_TOKEN("<?");
token_t token_tag_begin =         DEFINE_CONSTANT_STR_TOKEN("<");
token_t token_comment_end =       DEFINE_CONSTANT_STR_TOKEN("-->");
token_t token_tag_closing_end =   DEFINE_CONSTANT_STR_TOKEN("/>");
token_t token_header_end =        DEFINE_CONSTANT_STR_TOKEN("?>");
token_t token_tag_end =           DEFINE_CONSTANT_STR_TOKEN(">");


token_t *token_list[] = {
        &whitespace,
        &alphanum,
        &token_attr_value,
        &token_attr_equal,
        &token_comment_begin,
        &token_tag_closing_begin,
        &token_header_begin,
        &token_tag_begin,
        &token_comment_end,
        &token_tag_closing_end,
        &token_header_end,
        &token_tag_end,
        NULL
};


gsymbol_t attr_line[] = {
        T_WHITESPACE_MO,
        TOKEN___H("Attr Name", alphanum, set_attr_name),
        TOKEN("=", token_attr_equal),
        TOKEN___H("Attr Val", token_attr_value, set_attr_value),
        END_WH(accept_attr)
};

gsymbol_t *attr_line_pack[] = {
        attr_line,
        NULL
};

gsymbol_t rule_xml_tag_end_full[] = {
        TOKEN___H(">", token_tag_end, got_tag_open),
        T_WHITESPACE_MO,
        NONTERM_MO_("Recurse Tag", NULL),
        T_WHITESPACE_MO,
        TOKEN__OH("TagValue", alphanum, value), // TODO handle tag's value
        T_WHITESPACE_MO,
        TOKEN("</", token_tag_closing_begin),
        TOKEN___H("TagName", alphanum, set_tag_name),
        TOKEN(">", token_tag_end),
        END
};

gsymbol_t rule_xml_tag_end_empty[] = {
        T_WHITESPACE_MO,
        TOKEN("/>", token_tag_closing_end),
        END_WH(got_tag_open)
};

gsymbol_t *tag_end_pack[] = {
        rule_xml_tag_end_full,
        rule_xml_tag_end_empty,
        NULL
};

gsymbol_t rule_xml_tag[] = {
        T_WHITESPACE_MO,
        TOKEN___H("<", token_tag_begin, do_tag_reset),
        TOKEN___H("TagName", alphanum, set_tag_name),
        NONTERM_MO_("Attributes", attr_line_pack),
        NONTERM____("Tag Ending", tag_end_pack),
        END_WH(got_tag_close)
};

gsymbol_t *tag_pack[] = {
        rule_xml_tag,
        NULL
};

gsymbol_t rule_xml_document[] = {
        NONTERM_MO_("Tag", tag_pack),
        END
};

rule_rv_t egram4xml_parse_from_str(const char *input, unsigned len) {
    parsing_context_t pc;
    unsigned processed_bytes = 0;
    xml_parse_context_t xml_parse_state;
    memset(&xml_parse_state, 0, sizeof(xml_parse_state));

    egram_reset_parser(&pc);
    pc.token_list = token_list;
    pc.user_data = &xml_parse_state;

    rule_xml_tag_end_full[2].elems = tag_pack; // link recurse

    rule_rv_t rv = egram_process(&pc, rule_xml_document, input, len, &processed_bytes);

    return rv;
}

#ifdef XML_EGRAM_TEST

int test_grammar() {


    const char test_str[] ="<root_tag>"
                           "<expected_interface name=\"TLM1\" dev_path=\"/dev/ttyS1\"/>\n"
                           "\n"
                           "<bus name=\"nav_bus\" eq_channel=\"1\" max_topics=\"128\"/>\n"
                           "\n"
                           "<bus name=\"control_bus\" eq_channel=\"1\" max_topics=\"128\">\n"
                           "    <dir name=\"solution\"/>\n"
                           "</bus>\n"
                           "\n"
                           "<bus name=\"gcu_bus\" eq_channel=\"0\" max_topics=\"256\">\n"
                           "<event_queue buffer_size=\"4000\" size=\"16\"/>\n"
                           "    <dir name=\"hk\" eq_channel=\"16\"/>\n"
                           "    <dir name=\"nav\" eq_channel=\"16\"/>\n"
                           "    <dir name=\"cont\" eq_channel=\"16\"/>\n"
                           "</bus>"
                           "</root_tag>\n";

    return egram4xml_parse_from_str(test_str, sizeof(test_str));
}


int main(int argc, char *argv[]) {

    test_grammar();

    return 0;
}


#endif