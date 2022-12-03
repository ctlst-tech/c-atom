#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "lib/expat.h"
#include "../xml_priv.h"
#include "../xml.h"

static void XMLCALL
startElement(void *userData, const char *name, const char **atts)
{
    xml_parser_t *parser = (xml_parser_t*) userData;

    xml_node_t *nn = new_node(name);

    for (int i = 0; ( atts [i] != NULL ) && ( atts [i + 1] != NULL) ; i += 2) {
        xml_attr_t *a = new_attr(atts[i], atts[i + 1]);
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

static void XMLCALL
charDatahandler (void *userData, const XML_Char *s, int len)
{
    xml_parser_t *parser = (xml_parser_t*) userData;

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
            char *d = xml_realloc((void *)parser->current_parent->data, pos + len + 1);
            memcpy(d + pos, s, len);
            d[pos + len] = 0;
            parser->current_parent->data = d;
        }
    }
}

static void XMLCALL endElement(void *userData, const char *name) {
    xml_parser_t *parser = (xml_parser_t*) userData;
    parser->current_parent = parser->current_parent->parent;
    // TODO check closing tag?
}


static xml_rv_t expat_reset(xml_parser_t *xml_parser, int free_dom) {
    if (xml_parser == NULL)
        return xml_e_invarg;

    XML_Bool rv = XML_ParserReset(xml_parser->parser_data_handle, NULL);
    if (rv != XML_TRUE) {
        return xml_e_parser_reset;
    }

    if (free_dom) {
        // TODO implement freeing of DOM?
    }

    xml_parser->current_parent = NULL;
    xml_parser->root = NULL;

    xml_parser->src_file_path = NULL;

    XML_SetUserData(xml_parser->parser_data_handle, xml_parser);
    XML_SetElementHandler(xml_parser->parser_data_handle, startElement, endElement);
    XML_SetCharacterDataHandler(xml_parser->parser_data_handle, charDatahandler);
    return xml_e_ok;
}

xml_rv_t expat_init(xml_parser_t *xml_parser) {
    if (xml_parser == NULL)
        return xml_e_invarg;

    memset(xml_parser, 0, sizeof(*xml_parser));

    xml_parser->parser_data_handle = XML_ParserCreate("UTF-8");
    if (xml_parser->parser_data_handle == NULL) {
        return xml_e_parser_creation;
    }

    return expat_reset(xml_parser, 0);
}

void expat_destroy(xml_parser_t *xml_parser) {
    XML_ParserFree(xml_parser->parser_data_handle);
}


static xml_rv_t expat_parse(xml_parser_t *xml_parser, const char *s, int len, int isFinal ) {


    if (XML_Parse(xml_parser->parser_data_handle, s, len, isFinal ) == XML_STATUS_ERROR) {
        xml_err("XML parsing error %s at line %u\n",
                XML_ErrorString(XML_GetErrorCode (xml_parser->parser_data_handle)),
                (uint32_t) XML_GetCurrentLineNumber(xml_parser->parser_data_handle));
        return xml_e_dom_parsing;
    }

    return xml_e_ok;
}

int expat_get_current_line(xml_parser_t *xml_parser) {
    return ( int ) XML_GetCurrentLineNumber(xml_parser->parser_data_handle);
}

xml_rv_t expat_parse_from_file(const char *path, xml_node_t **parse_result_root) {
    if (path == NULL)
        return xml_e_invarg;

    xml_parser_t xml_parser;

    xml_rv_t rv;

#   define BUF_SIZE 4096
    char buff[BUF_SIZE];
    int fd = -1;

    do {
        rv = expat_init(&xml_parser);
        if (rv != xml_e_ok) {
            break;
        }

        if ((fd = open(path, O_RDONLY)) == -1) {
            switch (errno) {
                case ENOENT:
                    rv = xml_e_no_file;
                    break;

                default:
                    rv = xml_e_file_open_unkn;
                    break;
            }

            break;
        }

        expat_reset(&xml_parser, 0);
        xml_parser.src_file_path = path;
        ssize_t br;

        do {
            br = read(fd, buff, BUF_SIZE);
            if (br == -1) {
                rv = xml_e_file_read;
                break;
            }
            rv = expat_parse(&xml_parser, buff, (int) br, (br < BUF_SIZE) ? -1 : 0);
            if (rv != xml_e_ok) {
                break;
            }
        } while (br == BUF_SIZE);

        *parse_result_root = xml_parser.root;
    } while (0);

    if (fd > 0) {
        close(fd);
    }

    expat_destroy(&xml_parser);

    return rv;
}
