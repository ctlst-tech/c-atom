#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#include "lib/egram.h"
#include "../xml.h"

/*
 * TODO:
 *  1. implement string parsing for testing
 *  2. debug basic cases
 *  3. create lambda based input reader
 *  4. generalize interfaces with expat parser
 */

rule_rv_t egram4xml_parse_from_str(const char *input, unsigned len);

xml_rv_t xml_egram_parse_from_str(const char *str, xml_node_t **parse_result_root) {

    rule_rv_t rv = egram4xml_parse_from_str(str, strlen(str));

    return rv == r_match ? xml_e_ok : xml_e_dom_parsing;
}
