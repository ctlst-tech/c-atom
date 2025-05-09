#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


#include "egram_xml.h"

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


static void startElement(xml_dom_walker_state_t *parser, const char *name, const attr_t *atts) {

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

static void charDatahandler (xml_dom_walker_state_t *parser, const char *s, int len) {

    int non_space = 0;
    int i = 0;
    while ((non_space == 0) && (i < len)) {
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
    };

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

static void endElement(xml_dom_walker_state_t *parser, const char *name) {
    parser->current_parent = parser->current_parent->parent;
}

xml_rv_t xml_egram_parser_init(void *dhandle, egram4xml_parser_t **parser_rv) {
    static egram4xml_parser_t *parser = NULL;
    // FIXME for now we have just one parser per app, i.e. not thread safe

    if (parser == NULL) {
        parser = egram4xml_parser_allocate();
        if (parser != NULL) {
            egram4xml_parser_init(parser, startElement, charDatahandler, endElement);
        }
    }

    if (parser == NULL) {
        return xml_e_nomem;
    }

    egram4xml_parser_setup(parser, dhandle);
    *parser_rv = parser;

    return xml_e_ok;
}

xml_rv_t xml_egram_parse_from_str(const char *str, xml_node_t **parse_result_root) {

    egram4xml_parser_t *parser;

    xml_dom_walker_state_t dom_walker;
    memset(&dom_walker, 0, sizeof(dom_walker));

    xml_rv_t rv = xml_egram_parser_init(&dom_walker, &parser);
    if (rv != xml_e_ok) {
        return rv;
    }

    rule_rv_t rrv = egram4xml_parse_from_str(parser, str, strlen(str));
    *parse_result_root = dom_walker.root;

    return rrv == r_match ? xml_e_ok : xml_e_dom_parsing;
}

#define MAX_XML_FILE_SIZE (1024*16)
static uint8_t file_content[MAX_XML_FILE_SIZE];

static int load_file(const char *path) {

    int rv = -1;
    unsigned offset = 0;

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        if (errno == 0) {
            // happens in FreeRTOS
            return ENOENT;
        }
        return errno;
    }

    while((rv = read(fd, &file_content[offset], 1024)) > 0) {
        offset += rv;
        if (offset > MAX_XML_FILE_SIZE) {
            close(fd);
            return ENOMEM;
        }
    }

    return rv < 0 ? errno : 0;
}

xml_rv_t egram_parse_from_file(const char *path, xml_node_t **parse_result_root) {
    int rv = load_file(path);
    switch (rv) {
        case 0:
            break;

        case ENOMEM:
            return xml_e_nomem;

        case ENOENT:
            return xml_e_no_file;

        default:
            return xml_e_file_read;
    }

    return xml_egram_parse_from_str((char *)file_content, parse_result_root);
}

xml_rv_t xml_parse_from_file(const char *path, xml_node_t **parse_result_root) {
    return egram_parse_from_file(path, parse_result_root);
}
