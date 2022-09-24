//
// Created by goofy on 2/2/22.
//




#include <stdlib.h>

#include <string.h>
#include <stdint.h>

#include "expat/expat.h"
#include "xml.h"

typedef struct xml_parser {
    XML_Parser     expat_parser;
    xml_node_t     *root;
    xml_node_t     *current_parent;

    const char      *src_file_path;
} xml_parser_t;


static void* xml_alloc(size_t s) {
    return calloc(1, s);
}

static void xml_free(void *p) {
    free (p);
}

static void* xml_realloc(void *ptr, size_t s) {
    return realloc(ptr, s);
}

char* xml_strdup(const char *s) {
    return strdup(s);
}

static int calc_commas(const char *s) {
    int rv = 0;

    do {
        if (*s == ',') {
            rv++;
        }
    } while (*s++ != 0);

    return rv;
}

static int tokenize(char *s, char **token_list) {
    char* context = NULL;
#   define SEP ", "
    char* token = strtok_r(s, SEP, &context);

    int i = 0;

    while (token != NULL) {
        if (token_list != NULL) {
            token_list[i] = xml_strdup(token);
        }
        token = strtok_r(NULL, SEP, &context);
        i++;
    }

    if (token_list != NULL) {
        token_list[i] = NULL;
    }

    return i;
}

#define MAX_ATTR_STR_LEN 128

int xml_list_from_attr_size(const char *s) {
    char str_tmp[MAX_ATTR_STR_LEN + 1];
    if (strlen(s) > MAX_ATTR_STR_LEN) {
        return xml_e_invattr;
    }
    strncpy(str_tmp, s, MAX_ATTR_STR_LEN);
    return tokenize(str_tmp, NULL);
}

char **xml_list_from_attr_alloc(int el_num) {
    char **rv = xml_alloc((el_num + 1) * sizeof(rv));

    return rv;
}

/**
 *
 * @param s
 * @param token_list_rv if value under this pointer is NULL, there will be allocation with aproproate size
 * @return
 */
xml_rv_t xml_list_from_attr(const char *s, char ***token_list_rv) {

    int tokens_num = xml_list_from_attr_size(s);
    if (tokens_num <= 1) {
        return xml_e_invarg;
    }

    char str_tmp[MAX_ATTR_STR_LEN + 1];
    if (strlen(s) > MAX_ATTR_STR_LEN) {
        return xml_e_invattr;
    }
    strncpy(str_tmp, s, sizeof(str_tmp));

    char **token_list;

    if (*token_list_rv == NULL) {
        token_list = xml_list_from_attr_alloc(tokens_num);
        if (token_list == NULL) {
            return xml_e_nomem;
        }
    } else {
        token_list = *token_list_rv;
    }

    tokenize(str_tmp, token_list);

    *token_list_rv = token_list;

    return xml_e_ok;
}

static xml_node_t *new_node(const char *name) {
    xml_node_t *rv = xml_alloc(sizeof(xml_node_t));
    if (rv == NULL) {
        return NULL;
    }

    rv->name = xml_strdup(name);

    return rv;
}

static void node_add_attr(xml_node_t *n, xml_attr_t *a) {
    if (n->attrs_list == NULL) {
        n->attrs_list = a;
        n->attrs_list_tail = a;
    } else {
        n->attrs_list_tail->next_attr = a;
        n->attrs_list_tail = a;
    }
}

static xml_attr_t *new_attr(const char *name, const char *value) {
    xml_attr_t *rv = xml_alloc(sizeof(xml_attr_t));
    if (rv == NULL) {
        return NULL;
    }

    rv->name = xml_strdup(name);
    rv->value = xml_strdup(value);

    return rv;
}

static void node_add_child(xml_node_t *parent, xml_node_t *child) {
    if (parent->first_child == NULL) {
        parent->first_child = child;
        parent->last_child = child;
    } else {
        parent->last_child->next_sibling = child;
        parent->last_child = child;
    }

    child->parent = parent;
}

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


const char *xml_attr_value(xml_node_t *n, const char *name) {
    for (xml_attr_t *an = n->attrs_list; an != NULL; an = an->next_attr) {
        if (strcmp(an->name, name) == 0) {
            return an->value;
        }
    }

    return NULL;
}



static xml_rv_t xml_parser_reset(xml_parser_t *xml_parser, int free_dom) {
    if (xml_parser == NULL)
        return xml_e_invarg;

    XML_Bool rv = XML_ParserReset(xml_parser->expat_parser, NULL);
    if (rv != XML_TRUE) {
        return xml_e_parser_reset;
    }

    if (free_dom) {
        // TODO implement freeing of DOM
    }

    xml_parser->current_parent = NULL;
    xml_parser->root = NULL;

    xml_parser->src_file_path = NULL;

    // Инициализация expat
    XML_SetUserData(xml_parser->expat_parser, xml_parser);
    XML_SetElementHandler(xml_parser->expat_parser, startElement, endElement);
    XML_SetCharacterDataHandler(xml_parser->expat_parser, charDatahandler);
    return xml_e_ok;
}

xml_parser_t *xml_parser_alloc() {
    return xml_alloc(sizeof(xml_parser_t));
}
void xml_parser_dealloc(xml_parser_t *p) {
    xml_free(p);
}

xml_rv_t xml_parser_init(xml_parser_t *xml_parser) {
    if (xml_parser == NULL)
        return xml_e_invarg;

    xml_parser->expat_parser = XML_ParserCreate("UTF-8");
    if (xml_parser->expat_parser == NULL) {
        return xml_e_parser_creation;
    }

    return xml_parser_reset(xml_parser, 0);;
}

void xml_parser_destroy(xml_parser_t *xml_parser) {
    XML_ParserFree(xml_parser->expat_parser);
}

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

static xml_rv_t xml_parse(xml_parser_t *xml_parser, const char *s, int len, int isFinal ) {


    if (XML_Parse(xml_parser->expat_parser, s, len, isFinal ) == XML_STATUS_ERROR) {
        fprintf(stderr, "XML parsing error %s at line %u\n",
                   XML_ErrorString(XML_GetErrorCode (xml_parser->expat_parser)),
                   (uint32_t) XML_GetCurrentLineNumber(xml_parser->expat_parser));
        return xml_e_dom_parsing;
    }

    return xml_e_ok;
}

int xml_parse_get_current_line(xml_parser_t *xml_parser) {
    return ( int ) XML_GetCurrentLineNumber(xml_parser->expat_parser);
}

xml_rv_t xml_parse_from_file(const char *path, xml_node_t **parse_result_root) {
    if (path == NULL)
        return xml_e_invarg;


    xml_parser_t *xml_parser = xml_parser_alloc();
    if (xml_parser == NULL) {
        return xml_e_nomem;
    }

    xml_rv_t rv;

    char *buff = NULL;
    int fd = -1;

    do {
        rv = xml_parser_init(xml_parser);
        if (rv != xml_e_ok) {
            break;
        }

#   define BUF_SIZE 4096


        if ((fd = open(path, O_RDONLY)) == -1) {
            //fprintf(stderr, "opening \'%s\' error: %s\n", path, strerror(errno));
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

        if ((buff = malloc(BUF_SIZE)) == NULL) {
            //fprintf(stderr, "buffer malloc failed\n");
            rv = xml_e_nomem;
            break;
        }

        xml_parser_reset(xml_parser, 0);
        xml_parser->src_file_path = path;
        ssize_t br;

        do {
            br = read(fd, buff, BUF_SIZE);
            if (br == -1) {
                rv = xml_e_file_read;
                break;
            }
            rv = xml_parse(xml_parser, buff, (int) br, (br < BUF_SIZE) ? -1 : 0);
            if (rv != xml_e_ok) {
                break;
            }
        } while (br == BUF_SIZE);

        *parse_result_root = xml_parser->root;
    } while (0);

    if (buff != NULL) {
        xml_free(buff);
    }
    if (fd > 0) {
        close(fd);
    }

    xml_parser_dealloc(xml_parser);

    return rv;
}




int xml_node_name_eq(xml_node_t *n, const char *name) {
    return strcmp(n->name, name) == 0 ? -1 : 0;
}

xml_node_t *xml_node_find_child(xml_node_t *parent, char *name) {
    for (xml_node_t *n = parent->first_child; n != NULL; n = n->next_sibling) {
        if (xml_node_name_eq(n, name)) {
            return n;
        }
    }

    return NULL;
}

xml_node_t *xml_node_find_following_sibling(xml_node_t *sibling, char *name) {
    for (xml_node_t *n = sibling->next_sibling; n != NULL; n = n->next_sibling) {
        if (xml_node_name_eq(n, name)) {
            return n;
        }
    }

    return NULL;
}


int xml_node_count_siblings(xml_node_t *first_sibling, const char *name2cnt) {
    int rv = 0;
    for (xml_node_t *n = first_sibling; n != NULL; n = n->next_sibling) {
        if (name2cnt == NULL) {
            rv++;
        } else if (xml_node_name_eq(n, name2cnt)) {
            rv++;
        }
    }

    return rv;
}

int xml_node_count_attrs(xml_node_t *node) {
    int rv = 0;

    for (xml_attr_t *n = node->attrs_list; n != NULL; n = n->next_attr) {
        rv++;
    }

    return rv;
}

int xml_node_child_is_unique(xml_node_t *n, char *name) {
    xml_node_t *ch = xml_node_find_child(n, name);
    xml_node_t *nch = xml_node_find_following_sibling(ch, name);

    return ((ch != NULL) && (nch == NULL)) ? -1 : 0;
}


const char *xml_attr_value(xml_node_t *n, const char *name);

int xml_attr_int(xml_node_t *n, const char *aname, xml_rv_t *err) {
    const char *v = xml_attr_value(n, aname);
    if (v == NULL) {
        *err = xml_e_noattr;
        return 0;
    }

    int value;

    if (sscanf(v,"%d", &value) < 1) {
        *err = xml_e_invattr;
        return 0;
    }

    *err = xml_e_ok;
    return value;
}

const char *xml_attr_str(xml_node_t *n, const char *aname, xml_rv_t *err) {
    const char *value = xml_attr_value(n, aname);
    if (value == NULL) {
        *err = xml_e_noattr;
        return NULL;
    }

    if (value[0] == 0) {
        *err = xml_e_invattr;
        return NULL;
    }

    *err = xml_e_ok;
    return value;
}

int xml_attr_bool(xml_node_t *n, const char *aname, xml_rv_t *err) {
    const char *value = xml_attr_value(n, aname);
    if (value == NULL) {
        *err = xml_e_noattr;
        return 0;
    }
    int rv = 0;
    const char *true_vals[] = {"yes", "true", "1", NULL};

    for (int i = 0; true_vals[i] != NULL; i++) {
        if (strcasecmp(true_vals[i], value) == 0) {
            rv = -1;
        }
    }

    return rv;
}

int xml_dom_process_attr_parser_err(const char *attr_name, xml_node_t *node, xml_rv_t err_code) {
    int rv;
    switch(err_code) {
        case xml_e_noattr:
            xml_err("No mandatory \"%s\" attr for node \"%s\"", attr_name, node->name);
            rv = 1;
            break;

        case xml_e_invattr:
            xml_err("\"%s\" has invalid format (node \"%s\")", attr_name, node->name);
            rv = 1;
            break;

        default:
            xml_err("\"%s\" attr parsing unknown error (node \"%s\")", attr_name, node->name);
            rv = 1;
            break;

        case xml_e_ok:
            rv = 0;
            break;
    }

    return rv;
}
