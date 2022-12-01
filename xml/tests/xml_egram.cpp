#include <catch2/catch_all.hpp>
#include <iostream>
#include <string.h>
#include <regex>

#include "../xml.h"

extern "C" xml_rv_t xml_egram_parse_from_str(const char *str, xml_node_t **parse_result_root);

TEST_CASE("Empty") {
    const char *test_str = "";

}

TEST_CASE("Just root tag") {
    const char *test_str = "<root></root>";
    xml_node_t *dom_rv;
    xml_rv_t rv = xml_egram_parse_from_str(test_str, &dom_rv);
    REQUIRE(rv == xml_e_ok);
}

TEST_CASE("Root tag w doc header") {

}

TEST_CASE("With attributes") {
    const char *test_str = "<root><node attr1=\"attr1val\"></node></root>";
    xml_node_t *dom_rv;
    xml_rv_t rv = xml_egram_parse_from_str(test_str, &dom_rv);
    REQUIRE(rv == xml_e_ok);
}

TEST_CASE("With comments") {

}

TEST_CASE("Multiple nesting") {

}

TEST_CASE("Non closed token") {

}

