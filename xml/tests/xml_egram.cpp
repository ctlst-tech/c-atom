#include <catch2/catch_all.hpp>
#include <iostream>
#include <list>
#include <string.h>
#include <regex>
#include <utility>

#include "../xml.h"

extern "C" xml_rv_t xml_egram_parse_from_str(const char *str, xml_node_t **parse_result_root);

class Base {

public:
    std::string name;
    std::string value;
    Base() = default;
    Base(std::string name, std::string value): name(name), value(value) {}
};

class Attr: public Base {

public:
    Attr() = default;
    Attr(std::string name, std::string value): Base(name, value) {}
    std::string render() {
        return name + "=" + "\"" + value + "\"";
    }

    bool match_with(xml_attr_t *a) {
        bool name_match = name == std::string(a->name);
        bool value_match = value == std::string(a->value);

        return name_match & value_match;
    }
};

class Node: public Base{
    std::list<Attr *> attrs;
    std::list<Node *> children;
    Node *parent;

public:
    Node() = default;
    Node(std::string name, std::string value = ""): Base(name, value) {parent = NULL;}

    Node* add_node(Node *n) {
        children.push_back(n);
        n->parent = this;
        return n;
    }

    Node* add_attr(Attr *a) {
        attrs.push_back(a);
        return this;
    }

    virtual std::string render(unsigned ident = 0) {
        std::string rv;
        std::string identation = std::string(ident * 4, ' ');
        rv = identation + "<" + name;

        for (auto a : attrs) {
            rv += " " + a->render();
        }

        bool empty_tag = children.empty() && value.empty();

        if (empty_tag) {
            rv += "/>\n";
        } else {
            rv += ">\n";
            if (!value.empty()) {
                rv += value + "\n";
            }
            for (auto n : children) {
                rv += n->render(ident+1);
            }
            rv += "</" + name + ">\n";
        }

        return rv;
    }

    bool match_with(xml_node_t *n) {
        bool tag_name_match = name == std::string(n->name);
//        bool value_match = value == std::string(n->data);

        bool attr_num_match = attrs.size() == xml_node_count_attrs(n);

        bool attrs_match = true;
        if (attr_num_match) {
            xml_attr_t* a = n->attrs_list;

            for (auto iter_attr = attrs.begin();
            iter_attr != attrs.end() && a != NULL;
             iter_attr++, a = a->next_attr) {
                if (!(*iter_attr)->match_with(a)) {
                    attrs_match = false;
                    break;
                }
            }
        }

//        return tag_name_match & value_match & attr_num_match & attrs_match;
        return tag_name_match & attr_num_match & attrs_match;
    }

    void print() {
        std::cout << "BEGIN Printing rendered node \"" + name + "\" BEGIN\n";
        std::cout << render();
        std::cout << "END Printing rendered node \"" + name + "\" END\n";
    }
};

void parse_and_validate(Node &tree) {
    tree.print();
    std::string test_str = tree.render();
    xml_node_t *dom_rv;
    xml_rv_t rv = xml_egram_parse_from_str(test_str.c_str(), &dom_rv);

    REQUIRE(rv == xml_e_ok);
    REQUIRE(tree.match_with(dom_rv) == true);
}

TEST_CASE("XML by EGRAM Normal") {
    Node n = Node("root", "\n");
    SECTION("Empty") {
        FAIL("Not implemented");
    }
    SECTION("Just root tag") {
        parse_and_validate(n);
    }
    SECTION("Root tag w doc header") {
        FAIL("Not implemented");
    }

    n.add_node(new Node("node"))
            ->add_attr(new Attr("attr1", "val1"))
            ->add_attr(new Attr("attr2", "val2"));

    SECTION("With attributes") {
        parse_and_validate(n);
    }
    SECTION("With comments") {
        FAIL("Not implemented");
    }
    SECTION("Multiple nesting") {
        FAIL("Not implemented");
    }
    SECTION("Non closed token") {
        FAIL("Not implemented");
    }
}
