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

protected:
    Node *parent;
    bool matchable;

public:
    Node() = default;
    Node(std::string name, std::string value = ""): Base(name, value) {
        parent = NULL;
        matchable = true;
    }

    Node* add_node(Node *n) {
        children.push_back(n);
        n->parent = this;
        return n;
    }

    Node* add_attr(Attr *a) {
        attrs.push_back(a);
        return this;
    }

    unsigned matchable_children_num() {
        unsigned rv = 0;
        for(auto n : children) {
            if (n->matchable) {
                rv++;
            }
        }

        return rv;
    }

    unsigned depth() {
        unsigned rv = 0;
        for (auto n = parent; n != NULL; n = n->parent) {
            rv++;
        }

        return rv;
    }

    std::string path() {
        std::string rv = name;
        for (auto n = parent; n != NULL; n = n->parent) {
            rv = parent->name + "/" + rv;
        }

        return rv;
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
            xml_attr_t* dom_a = n->attrs_list;

            for (auto iter_attr = attrs.begin();
            iter_attr != attrs.end() && dom_a != NULL;
             iter_attr++, dom_a = dom_a->next_attr) {
                if (!(*iter_attr)->match_with(dom_a)) {
                    attrs_match = false;
                    break;
                }
            }
        }
        bool nodes_match = true;
        bool nodes_num_match = matchable_children_num() == xml_node_count_siblings(n->first_child, NULL);
        xml_node_t * dom_n = n->first_child;

        if (nodes_num_match) {
            for (auto iter_node = children.begin();
                 iter_node != children.end() && dom_n != NULL;
                 iter_node++) {
                if ((*iter_node)->matchable) {
                    if (!(*iter_node)->match_with(dom_n)) {
                        nodes_match = false;
                        break;
                    }
                    dom_n = dom_n->next_sibling;
                }
            }
        }

//        return tag_name_match & value_match & attr_num_match & attrs_match;
        bool rv = tag_name_match & attr_num_match & attrs_match & nodes_num_match & nodes_match;
        std::string identation = std::string(depth() * 4, '_');

        #define MS(v__) ((v__) ? "MATCH" : "DIFF")
        #define PRINT(n__,v__) {std::cerr << identation << (n__) << " " << MS(v__) << std::endl;}

        if (!rv) {
            std::cerr << identation << name << std::endl;
            PRINT("Name     ", tag_name_match);
            PRINT("Attr num ", attr_num_match);
            PRINT("Attrs    ", attrs_match);
            PRINT("Nodes num", nodes_num_match);
            PRINT("Nodes    ", nodes_match);
        }

        return rv;
    }

    void print() {
        std::cout << "BEGIN Printing rendered node \"" + name + "\" BEGIN\n";
        std::cout << render();
        std::cout << "END Printing rendered node \"" + name + "\" END\n";
    }
};

class Comment: public Node {
    std::string comment;

public:
    Comment() = default;
    Comment(std::string comment_): Node(), comment(comment_) {
        parent = NULL;
        matchable = false;
    }

    std::string render(unsigned ident = 0) {
        std::string identation = std::string(ident * 4, ' ');

        return identation + "<!-- " + comment + " -->\n";
    }
};

void parse_and_validate(Node &tree) {
    tree.print();
    std::string test_str = tree.render();
    xml_node_t *dom_rv;
    xml_rv_t rv = xml_egram_parse_from_str(test_str.c_str(), &dom_rv);

    CHECK(rv == xml_e_ok);
    bool match = tree.match_with(dom_rv);
    REQUIRE(match == true);
}

TEST_CASE("XML by EGRAM Normal") {
    Node n = Node("root", "\n");
    SECTION("Empty") {
        FAIL("Not implemented");
    }
    SECTION("Just root tag") {
        parse_and_validate(n);
    }

    auto nn = n.add_node(new Node("node"));

    SECTION("Multiple nesting") {
        parse_and_validate(n);
    }

    nn->add_attr(new Attr("attr1", "val1"))
        ->add_attr(new Attr("attr2", "val2"));

    SECTION("With attributes") {
        parse_and_validate(n);
    }

    n.add_node(new Comment("random comment to break the parsing"));

    SECTION("With comments") {
        parse_and_validate(n);
    }

    n.add_node(new Node("after_comment_node"));

    SECTION("With nodes after comment") {
        parse_and_validate(n);
    }

    for (auto i : {0, 10}) {
        n.add_node(new Node("node"))
            ->add_node(new Node("subnode"))
            ->add_attr(new Attr("a", "v" + std::to_string(i)));
    }

    SECTION("Multiple nodes and attrs") {
        parse_and_validate(n);
    }

    SECTION("Root tag w doc header") {
        FAIL("Not implemented");
    }

//    SECTION("Non closed token") {
//        FAIL("Not implemented");
//    }
}
