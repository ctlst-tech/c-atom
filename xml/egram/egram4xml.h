#ifndef C_ATOM_EGRAM4XML_H
#define C_ATOM_EGRAM4XML_H

#include "lib/egram.h"
#include "../xml_priv.h"

typedef struct egram4xml_parser {
    egram_parsing_context_t pc;
} egram4xml_parser_t;

typedef struct {
    const char *name;
    const char *value;
} attr_t;

void egram4xml_parser_init(egram4xml_parser_t *parser,
                           xml_parser_t *data_handle,
                           void (*start_element)(xml_parser_t *parser, const char *name, const attr_t *atts),
                           void (*char_datahandler) (xml_parser_t *parser, const char *s, int len),
                           void (*end_element)(xml_parser_t *parser, const char *name));

rule_rv_t egram4xml_parse_from_str(egram4xml_parser_t *parser, const char *input, unsigned len);

#endif //C_ATOM_EGRAM4XML_H
