#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#include "egram4xml.h"

/*
 * TODO:
 *  1. implement string parsing for testing
 *  2. debug basic cases
 *  3. create lambda based input reader
 *  4. generalize interfaces with expat parser
 *  5. fix problems
 *   [ ] tag's value concatination
 *   [ ] handle attr overflow
 *   [ ] handle attr tag value overflow
 *   [ ] pass parsing errors: unexpected token,
 *  6. pass line and pos num errors
 */


static void startElement(xml_parser_t *parser, const char *name, const attr_t *atts) {

    xml_node_t *nn = new_node(name);

    for (int i = 0; ( atts [i].name != NULL ) && ( atts [i].value != NULL) ; i++) {
        xml_attr_t *a = new_attr(atts[i].name, atts[i].value);
        if (a != NULL) {
            node_add_attr(nn, a);
        }
    }

    do {
        if (parser->root == NULL) {
            parser->root = nn;
            parser->current_parent = nn;
        } else {
            if (parser->current_parent == NULL) {
                // TODO another root in file
                break;
            }

            node_add_child(parser->current_parent, nn);
            parser->current_parent = nn;
        }
    } while (0);
}

static void charDatahandler (xml_parser_t *parser, const char *s, int len) {

    int non_space = 0;
    int i = 0;
    do {
        switch (s[i]) {
            case '\n':
            case '\r':
            case ' ':
            case '\t':
                break;

            default:
                non_space++;
        }

        i++;
    } while((non_space == 0) && (i < len));

    if (non_space > 0) {

        if (parser->current_parent->data == NULL) {
            char *d = xml_alloc(len + 1);
            if (d != NULL) {
                memcpy(d, s, len);
                d[len] = 0;
            }
            parser->current_parent->data = d;
        } else {
            // handling situation, where we need to concat string data due to consecutive charDatahandler call

            size_t pos = strlen(parser->current_parent->data);
            // FIXME
            char *d = xml_realloc((void *)parser->current_parent->data, pos + len + 1);
            memcpy(d + pos, s, len);
            d[pos + len] = 0;
            parser->current_parent->data = d;
        }
    }
}

static void endElement(xml_parser_t *parser, const char *name) {
    parser->current_parent = parser->current_parent->parent;
}


xml_rv_t xml_egram_parse_from_str(const char *str, xml_node_t **parse_result_root) {

    static egram4xml_parser_t parser;
    xml_parser_t dom_data;

    memset(&dom_data, 0, sizeof(dom_data));

    egram4xml_parser_init(&parser, &dom_data, startElement, charDatahandler, endElement);

    rule_rv_t rv = egram4xml_parse_from_str(&parser, str, strlen(str));
    *parse_result_root = dom_data.root;

    return rv == r_match ? xml_e_ok : xml_e_dom_parsing;
}
