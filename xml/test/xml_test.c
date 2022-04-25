//
// Created by goofy on 2/2/22.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xml.h"

void print_dom_r(xml_node_t *n2p, int depth) {
    printf("%*c",depth * 2, ' '); printf("Node: \"%s\" data: \"%s\"\n", n2p->name,
                                            (n2p->data == NULL) ? "" :  n2p->data);
    printf("%*c",depth * 2, ' '); printf("  Attrs: ");
        for (xml_attr_t *a = n2p->attrs_list; a != NULL; a = a->next_attr) {
            printf("%s=\"%s\"; ", a->name, a->value);
        }
        printf("\n");

    for (xml_node_t *n = n2p->first_child; n != NULL; n = n->next_sibling) {
        print_dom_r(n, depth+1);
    }
}

void print_dom(xml_node_t *n) {
    print_dom_r(n, 0);
}

int main(int argc, char *argv[]) {

    xml_node_t *dom;
    xml_rv_t rv = xml_parse_from_file("../../tests/flow_cfg.xml", &dom);

    print_dom(dom);

    return 0;
}
