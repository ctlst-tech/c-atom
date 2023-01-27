#ifndef C_ATOM_XML_H
#define C_ATOM_XML_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct xml_node;
struct xml_attr;

typedef enum {
    xml_e_ok = 0,
    xml_e_invarg,
    xml_e_nomem,
    xml_e_parser_creation,
    xml_e_parser_reset,
    xml_e_dom_parsing,
    xml_e_no_file,
    xml_e_file_open_unkn,
    xml_e_file_read,
    xml_e_noattr,
    xml_e_invattr,
    xml_e_dom_process,
    xml_e_dom_empty,
} xml_rv_t;

typedef struct xml_attr {
    const char *name;
    const char *value;
    struct xml_attr *next_attr;
} xml_attr_t;

typedef struct xml_node {
    const char *name;
    const char *data;

    xml_attr_t *attrs_list;
    xml_attr_t *attrs_list_tail;

    struct xml_node *parent;
    struct xml_node *first_child;
    struct xml_node *last_child;
    struct xml_node *next_sibling;

} xml_node_t;

xml_rv_t xml_parse_from_file(const char *path, xml_node_t **parse_result_root);

int xml_node_name_eq(xml_node_t *n, const char *name);

xml_node_t *xml_node_find_child(xml_node_t *parent, char *name);

xml_node_t *xml_node_find_following_sibling(xml_node_t *sibling, char *name);

int xml_node_count_siblings(xml_node_t *first_sibling, const char *name2cnt);

int xml_node_count_children(xml_node_t *parent, const char *name2cnt);

int xml_node_count_attrs(xml_node_t *node);

int xml_node_child_is_unique(xml_node_t *n, char *name);

const char *xml_attr_value(xml_node_t *n, const char *name);

int xml_attr_int(xml_node_t *n, const char *aname, xml_rv_t *err);

double xml_attr_double(xml_node_t *n, const char *aname, xml_rv_t *err);

const char *xml_attr_str(xml_node_t *n, const char *aname, xml_rv_t *err);

int xml_attr_bool(xml_node_t *n, const char *aname, xml_rv_t *err);

xml_rv_t xml_list_from_attr(const char *s, const char ***token_list_rv);

int xml_list_from_attr_size(const char *s);

const char **xml_list_from_attr_alloc(int el_num);

const char *xml_strdup(const char *s);

#define xml_err(text, ...) fprintf(stderr, text "\n", ##__VA_ARGS__)

int xml_dom_process_attr_parser_err(const char *attr_name, xml_node_t *node, xml_rv_t err_code, int opt);

const char *xml_strerror(xml_rv_t ec);

#define GET_ATTR_INIT()     xml_rv_t ___xml_rv_;                             \
                            int ____cnt = 0;

#define GET_ATTR_AND_PROCESS_ERR_GENERIC(_func, _node, _attr_name, _opt, _err_cnt_ptr)   \
        _func(_node, _attr_name, &___xml_rv_);                                           \
        ____cnt += xml_dom_process_attr_parser_err(_attr_name, _node, ___xml_rv_, _opt); \
        if ((_err_cnt_ptr) != NULL) {*(_err_cnt_ptr) = ____cnt;}


#define GET_ATTR_AND_PROCESS_ERR(_func, _node, _attr_name, _err_cnt_ptr) GET_ATTR_AND_PROCESS_ERR_GENERIC(_func,_node,_attr_name,0,_err_cnt_ptr)
#define GET_ATTR_OPTIONAL(_func, _node, _attr_name, _err_cnt_ptr) GET_ATTR_AND_PROCESS_ERR_GENERIC(_func,_node,_attr_name,-1,_err_cnt_ptr)

#ifdef __cplusplus
}
#endif

#endif //C_ATOM_XML_H
