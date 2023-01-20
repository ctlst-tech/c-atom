#include <stdlib.h>

#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <ctype.h>


#include "xml.h"
#include "xml_priv.h"



void* xml_alloc(size_t s) {
    return calloc(1, s);
}

void xml_free(void *p) {
    free (p);
}

void* xml_realloc(void *ptr, size_t s) {
    return realloc(ptr, s);
}



#define STR_LOOKUP_BATCH_SIZE 200
/*
 * FIXME not thread safe
 * FIXME make it O(log N)
 */

//typedef struct str_lookup_batch {
const char *strs_lookup_batch[STR_LOOKUP_BATCH_SIZE];
unsigned strs_lookup_batch_size = 0;
unsigned dyn_mem_saved = 0;

const char *lookup_str(const char *s) {

    for (unsigned i = 0; i < strs_lookup_batch_size; i++) {
        if (strcmp(strs_lookup_batch[i], s) == 0) {
            return strs_lookup_batch[i];
        }
    }

    return NULL;
}

int add_to_lookup_table(const char *s) {
    if (strs_lookup_batch_size >= STR_LOOKUP_BATCH_SIZE) {
        return -1;
    }

    strs_lookup_batch[strs_lookup_batch_size] = s;
    strs_lookup_batch_size++;
    return 0;
}

const char* xml_strdup(const char *s) {
    const char *rv = lookup_str(s);
    if (rv == NULL) {
        rv = strdup(s);
        if (add_to_lookup_table(rv)) {
            dyn_mem_saved += strlen(s);
        }
    }
    return rv;
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

static int tokenize(char *s, const char **token_list) {
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

/**
 * Calculates size of the potential list created by tokenization s with space and comma separators
 * @param s string to see elements in
 * @return number of the tokens inside s
 */
int xml_list_from_attr_size(const char *s) {
    char str_tmp[MAX_ATTR_STR_LEN + 1];
    if (strlen(s) > MAX_ATTR_STR_LEN) {
        return xml_e_invattr;
    }
    strncpy(str_tmp, s, MAX_ATTR_STR_LEN);
    return tokenize(str_tmp, NULL);
}

const char **xml_list_from_attr_alloc(int el_num) {
    return xml_alloc((el_num + 1) * sizeof(const char **));
}

/**
 *
 * @param s
 * @param token_list_rv if value under this pointer is NULL, there will be allocation with aproproate size
 * @return
 */
xml_rv_t xml_list_from_attr(const char *s, const char ***token_list_rv) {

    int tokens_num = xml_list_from_attr_size(s);
    if (tokens_num <= 1) {
        return xml_e_invarg;
    }

    char str_tmp[MAX_ATTR_STR_LEN + 1];
    if (strlen(s) > MAX_ATTR_STR_LEN) {
        return xml_e_invattr;
    }
    strncpy(str_tmp, s, sizeof(str_tmp));

    const char **token_list;

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

xml_node_t *new_node(const char *name) {
    xml_node_t *rv = xml_alloc(sizeof(xml_node_t));
    if (rv == NULL) {
        return NULL;
    }

    rv->name = xml_strdup(name);

    return rv;
}

void node_add_attr(xml_node_t *n, xml_attr_t *a) {
    if (n->attrs_list == NULL) {
        n->attrs_list = a;
        n->attrs_list_tail = a;
    } else {
        n->attrs_list_tail->next_attr = a;
        n->attrs_list_tail = a;
    }
}

xml_attr_t *new_attr(const char *name, const char *value) {
    xml_attr_t *rv = xml_alloc(sizeof(xml_attr_t));
    if (rv == NULL) {
        return NULL;
    }

    rv->name = xml_strdup(name);
    rv->value = xml_strdup(value);

    return rv;
}

void node_add_child(xml_node_t *parent, xml_node_t *child) {
    if (parent->first_child == NULL) {
        parent->first_child = child;
        parent->last_child = child;
    } else {
        parent->last_child->next_sibling = child;
        parent->last_child = child;
    }

    child->parent = parent;
}


const char *xml_attr_value(xml_node_t *n, const char *name) {
    for (xml_attr_t *an = n->attrs_list; an != NULL; an = an->next_attr) {
        if (strcmp(an->name, name) == 0) {
            return an->value;
        }
    }

    return NULL;
}


xml_dom_walker_state_t *xml_parser_alloc() {
    return xml_alloc(sizeof(xml_dom_walker_state_t));
}

void xml_parser_dealloc(xml_dom_walker_state_t *p) {
    xml_free(p);
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

/**
 *
 * @param first_sibling
 * @param name2cnt if NULL all sibling will be counted
 * @return
 */
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

/**
 *
 * @param parent
 * @param name2cnt if NULL all sibling will be counted
 * @return
 */
int xml_node_count_children(xml_node_t *parent, const char *name2cnt) {
    return xml_node_count_siblings(parent->first_child, name2cnt);
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

int str_is_zero_num(const char *s) {
    const char *c = s;
    while(*c++ != 0) {
        switch (*c) {
            case '0':
            case '+':
            case '-':
            case 'x':
                break;

            default:
                return 0;
        }
    }

    return 1;
}

int xml_attr_int(xml_node_t *n, const char *aname, xml_rv_t *err) {
    const char *v = xml_attr_value(n, aname);
    if (v == NULL) {
        *err = xml_e_noattr;
        return 0;
    }

    int value = strtol(v, NULL, 0);
    if (errno != 0
        || (value == 0 && !str_is_zero_num(v))) {
        *err = xml_e_invattr;
        return 0;
    }

    *err = xml_e_ok;
    return value;
}


double xml_attr_double(xml_node_t *n, const char *aname, xml_rv_t *err) {
    const char *v = xml_attr_value(n, aname);
    if (v == NULL) {
        *err = xml_e_noattr;
        return 0;
    }

    double value = 0;

    if (sscanf(v,"%lf", &value) < 1) {
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

const char *xml_strerror(xml_rv_t ec) {
    switch(ec) {
        case xml_e_ok:                  return "Ok";
        case xml_e_invarg:              return "Invalid argument";
        case xml_e_nomem:               return "No memory";
        case xml_e_parser_creation:     return "Parser creation failed";
        case xml_e_parser_reset:        return "Parser reset";
        case xml_e_dom_parsing:         return "DOM parsing error";
        case xml_e_no_file:             return "No file";
        case xml_e_file_open_unkn:      return "Unknown opening error";
        case xml_e_file_read:           return "File read error";
        case xml_e_noattr:              return "No attribute";
        case xml_e_invattr:             return "Invalid attribute";
        case xml_e_dom_process:         return "DOM process error";
        case xml_e_dom_empty:           return "DOM empty";
        default:                        return "Unknown code";
    }
}
