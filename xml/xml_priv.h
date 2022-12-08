#ifndef C_ATOM_XML_PRIV_H
#define C_ATOM_XML_PRIV_H

#include "xml.h"

typedef struct xml_parser {
    void           *parser_data_handle;
    xml_node_t     *root;
    xml_node_t     *current_parent;
    const char     *src_file_path;
} xml_dom_walker_state_t;

xml_node_t *new_node(const char *name);
void node_add_attr(xml_node_t *n, xml_attr_t *a);
xml_attr_t *new_attr(const char *name, const char *value) ;
void node_add_child(xml_node_t *parent, xml_node_t *child);

void* xml_alloc(size_t s);
void xml_free(void *p);
void* xml_realloc(void *ptr, size_t s);

#endif //C_ATOM_XML_PRIV_H
