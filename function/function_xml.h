//
// Created by goofy on 2/3/22.
//

#ifndef C_ATOM_FUNCTION_XML_H
#define C_ATOM_FUNCTION_XML_H

#include "xml.h"
#include "function.h"

xml_rv_t func_xml_load_spec(xml_node_t* spec_node, function_spec_t *func_spec);
xml_rv_t func_xml_load_connections(xml_node_t* n, const char *tag, connection_spec_t ***spec);

xml_rv_t func_load_inputs(xml_node_t *conn_node, func_conn_spec_t **spec);
xml_rv_t func_load_outputs(xml_node_t *conn_node, func_conn_spec_t **spec);
xml_rv_t func_load_params(xml_node_t *conn_node, func_param_t **spec);

#endif //C_ATOM_FUNCTION_XML_H
