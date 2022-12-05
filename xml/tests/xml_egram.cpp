#include <catch2/catch_all.hpp>
#include <iostream>
#include <list>
#include <string.h>
#include <regex>
#include <utility>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>

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

std::string remove_char(std::string str, char c) {
    str.erase(std::remove(str.begin(), str.end(), c), str.end());

    return str;
}

std::string identation(unsigned ident) {
    return std::string(ident * 4, ' ');
}


bool replace_first(
        std::string& s,
        std::string const& toReplace,
        std::string const& replaceWith
) {
    std::size_t pos = s.find(toReplace);
    if (pos == std::string::npos) return false;
    s.replace(pos, toReplace.length(), replaceWith);
    return true;
}

std::string remove_nrts_chars(std::string str) {

    str = remove_char(str, '\n');
    str = remove_char(str, '\r');
    str = remove_char(str, '\t');

    auto idnt = identation(1);
    while(replace_first(str, idnt, ""));

    return str;
}

class Node: public Base{
    std::list<Attr *> attrs;
    std::list<Node *> children;
    std::string xml_header;

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

    void add_xml_header() {
        if (parent == NULL) {
            xml_header = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>";
        } else {
            throw NULL;
        }
    }

    void add_text_to_header(std::string s) {
        if (parent == NULL) {
            xml_header += s;
        } else {
            throw NULL;
        }
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

        if (!xml_header.empty()) {
            rv += xml_header + "\n";
        }

        rv += identation(ident) + "<" + name;

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
            rv += identation(ident) + "</" + name + ">\n";
        }

        return rv;
    }

    bool match_with(xml_node_t *n) {
        if (n == NULL) {
            return false;
        }
        bool tag_name_match = name == std::string(n->name);
        bool value_match = remove_nrts_chars(value)
                           == remove_nrts_chars(n->data != NULL ? std::string(n->data) : "");

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
        bool rv = tag_name_match & value_match & attr_num_match & attrs_match & nodes_num_match & nodes_match;
        std::string identation = std::string(depth() * 4, '_');

        #define MS(v__) ((v__) ? "MATCH" : "DIFF")
        #define PRINT(n__,v__) {std::cerr << identation << (n__) << " " << MS(v__) << std::endl;}

        if (!rv) {
            std::cerr << identation << name << std::endl;
            PRINT("Name     ", tag_name_match);
            PRINT("Value     ", value_match);
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

Node *nodes_tree_from_dom(xml_node_t *dom_el) {
    Node *N = new Node(std::string(dom_el->name),
                       dom_el->data != NULL ? std::string(dom_el->data) : "");

    for (xml_attr_t *attr = dom_el->attrs_list; attr != NULL; attr = attr->next_attr) {
        N->add_attr(new Attr(std::string(attr->name), std::string(attr->value)));
    }
    for (xml_node_t *n = dom_el->first_child; n != NULL; n = n->next_sibling) {
        auto nn = nodes_tree_from_dom(n);
        N->add_node(nn);
    }

    return N;
}

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
    Node root_node = Node("root", "\n");
//    SECTION("Empty") {
//        FAIL("Not implemented");
//    }
    SECTION("Just root tag") {
        parse_and_validate(root_node);
    }

    auto nn = root_node.add_node(new Node("node"));

    SECTION("Multiple nesting") {
        parse_and_validate(root_node);
    }

    nn->add_attr(new Attr("attr1", "val1"))
        ->add_attr(new Attr("attr2", "val2"));

    SECTION("With attributes") {
        parse_and_validate(root_node);
    }

    root_node.add_node(new Comment("random comment to break the parsing"));

    SECTION("With comment") {
        parse_and_validate(root_node);
    }

    root_node.add_node(new Comment("random comment to break the parsing 2"));
    root_node.add_node(new Comment("random comment to break the parsing 3"));

    SECTION("With several comments") {
        parse_and_validate(root_node);
    }

    root_node.add_node(new Node("after_comment_node"));

    SECTION("With nodes after comment") {
        parse_and_validate(root_node);
    }

    for (auto i : {0, 10}) {
        root_node.add_node(new Node("node"))
            ->add_node(new Node("subnode"))
            ->add_attr(new Attr("a", "v" + std::to_string(i)));
    }

    SECTION("Multiple nodes and attrs") {
        parse_and_validate(root_node);
    }

    root_node.add_xml_header();

    SECTION("With doc header") {
        parse_and_validate(root_node);
    }

    auto nnn = nn;

    for (auto i : {0, 4}) {
        nnn = nnn->add_node(new Node("node" + std::to_string(i)));
        for (auto j : {0, 10}) {
            nnn->add_attr(new Attr("attr" + std::to_string(j), "value" + std::to_string(j) ));
        }
    }

    SECTION("Multiple nesting") {
        parse_and_validate(root_node);
    }

    nn->value = ",.:{}_-=@#$%^*()";

    SECTION("With various chars in values") {
        parse_and_validate(root_node);
    }

    root_node.add_text_to_header("\n<!-- random comment -->\n");

    SECTION("With comment after XML header") {
        parse_and_validate(root_node);
    }

    root_node.add_text_to_header("\n\n\t\t<!-- random comment 2 -->\n\n");

    SECTION("With another comment after XML header") {
        parse_and_validate(root_node);
    }
}

bool file_to_str(const char *path, std::string &content) {
    #define BUF_SIZE 4096
    char buf[BUF_SIZE + 1];
    int rv;

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        std::cerr << "File \"" << path <<"\" opening error: " << strerror(errno);
        return false;
    }
    content = "";

    while((rv = read(fd, buf, BUF_SIZE)) > 0) {
        buf[rv] = 0;
        content += std::string(buf);
    }

    return true;
}

xml_rv_t egram_parse_from_file_test(const char *path, xml_node_t **parse_result_root) {

    std::string file_content;

    if (!file_to_str(path, file_content)) {
        return xml_e_no_file;
    }

    return xml_egram_parse_from_str(file_content.c_str(), parse_result_root);
}

extern "C" xml_rv_t expat_parse_from_file(const char *path, xml_node_t **parse_result_root);

TEST_CASE("Real files as single string testing") {

    const char *files[] = {
        "config/cube/flow_cont_angpos.xml",
        "config/cube/flow_cont_angrate.xml",
        "config/cube/flow_housekeeping.xml",
        "config/cube/flow_nav_attitude_filter.xml",
        "config/cube/flow_nav_attitude_prop.xml",
        "config/cube/flow_nav_imu_alignment.xml",
        "config/cube/flow_rc.xml",
        "config/cube/swsys.xml",
    };

    for (auto f : files) {
        SECTION(f) {
            xml_node_t *expat_dom;
            xml_node_t *egram_dom;

            auto expat_start = std::chrono::high_resolution_clock::now();
            xml_rv_t rv = expat_parse_from_file(f, &expat_dom);
            auto expat_stop = std::chrono::high_resolution_clock::now();
            REQUIRE(rv == xml_e_ok);

            Node *expat_root_node = nodes_tree_from_dom(expat_dom);
            REQUIRE(expat_root_node != NULL);

            auto egram_start = std::chrono::high_resolution_clock::now();
            rv = egram_parse_from_file_test(f, &egram_dom);
            auto egram_stop = std::chrono::high_resolution_clock::now();
            REQUIRE(rv == xml_e_ok);

            bool match = expat_root_node->match_with(egram_dom);
            REQUIRE(match == true);

            auto expat_duration = std::chrono::duration_cast<std::chrono::microseconds>(expat_stop - expat_start);
            auto egram_duration = std::chrono::duration_cast<std::chrono::microseconds>(egram_stop - egram_start);

            std::cout << "expat " << expat_duration.count() << " egram " << egram_duration.count() << " | file " << f << std::endl;
        }
    }
}
