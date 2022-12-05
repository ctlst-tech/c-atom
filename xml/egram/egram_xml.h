#ifndef C_ATOM_EGRAM_XML_H
#define C_ATOM_EGRAM_XML_H

#include "lib/egram.h"
#include "../xml_priv.h"

typedef struct egram4xml_parser {
    egram_parsing_context_t pc;
} egram4xml_parser_t;

typedef struct {
    const char *name;
    const char *value;
} attr_t;

egram4xml_parser_t *egram4xml_parser_allocate();

void egram4xml_parser_init(egram4xml_parser_t *parser,
                           void (*start_element)(xml_dom_walker_state_t *, const char *, const attr_t *),
                           void (*char_datahandler)(xml_dom_walker_state_t *, const char *, int),
                           void (*end_element)(xml_dom_walker_state_t *, const char *));

void egram4xml_parser_setup(egram4xml_parser_t *parser,
                            xml_dom_walker_state_t *data_handle);

rule_rv_t egram4xml_parse_from_str(egram4xml_parser_t *parser, const char *input, unsigned len);

#endif //C_ATOM_EGRAM_XML_H
