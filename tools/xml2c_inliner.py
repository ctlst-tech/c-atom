#!/usr/bin/env python3

import glob
import sys
import os.path
from typing import *
import pathlib


def new_file_write_call(path):
    file = open(path, 'w')
    def fprint(*args, **kwargs):
        print(*args, **kwargs, file=file)
    return fprint


inline_record_typename = 'xml_inline_record_t'
inline_registry_varname = 'xml_inline_registry'


def collect_xmls(root_path: str):
    files = list(pathlib.Path(root_path).glob('**/*.xml'))

    return files


def write_header(fprint):
    fprint(f'#include <stdlib.h>')
    fprint(f'#include <string.h>')
    fprint(f'')
    fprint(f'')
    fprint(f'typedef struct {{')
    fprint(f'    const char *path;')
    fprint(f'    const char *file;')
    fprint(f'    size_t size;')
    fprint(f'}} {inline_record_typename};')
    fprint(f'')
    fprint(f'')


def write_reg_finder(fprint):
    fprint(f'const char *xml_inline_find_file(const char *path, size_t *size) {{')
    fprint(f'    while(*path == \'/\') path++;')
    fprint(f'    for (int i = 0; {inline_registry_varname}[i] != NULL; i++) {{')
    fprint(f'        if (strcmp(path, {inline_registry_varname}[i]->path) == 0) {{')
    fprint(f'            *size = {inline_registry_varname}[i]->size;')
    fprint(f'            return {inline_registry_varname}[i]->file;')
    fprint(f'        }}')
    fprint(f'    }}')
    fprint(f'')
    fprint(f'    return NULL;')
    fprint(f'}}')


def write_xml(fprint, xml_lines, rel_path: str):
    varname = 'xml_inline_' + rel_path.replace('/', '_').replace('.', '_').replace('..', '')
    fprint(f'static const {inline_record_typename} {varname} = {{')
    fprint(f'    .path = \"{rel_path}\",')
    fprint(f'    .file = \\')

    size = 0

    for line in xml_lines:
        if (len(line) > 0):
            # be carefull regarding changing the alg to calc size
            # the size of the string will be validated at the opening phase
            size += len(line)
            ltw = line.replace('\"', '\\\"')
            fprint(f'        \"{ltw}\" \\')
    fprint(f'        \"\",')

    fprint(f'    .size = {size}')

    fprint(f'}};')
    fprint(f'')
    fprint(f'')


    return varname


def gen_code(c_fprint, xmls2inline, root_path):
    file_consts = []
    write_header(c_fprint)

    for xml_path in xmls2inline:
        with open(xml_path) as f:
            lines = [line.rstrip() for line in f]
            n = write_xml(c_fprint, lines, os.path.relpath(xml_path, root_path))
            print(f'Processing {xml_path}')

        file_consts.append(n)

    files_num = len(file_consts)
    c_fprint(f'static const {inline_record_typename} *{inline_registry_varname} [{files_num+1}] = {{')

    for c in file_consts:
        c_fprint(f'        &{c},')

    c_fprint(f'        NULL')

    c_fprint(f'}};')
    c_fprint(f'')
    c_fprint(f'')

    write_reg_finder(c_fprint)

    return files_num


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser('ESWB monitor tool')

    parser.add_argument(
        '--cfg_path',
        action='store',
        type=str,
        help='path to XML configurations catalog',
    )

    parser.add_argument(
        '--out',
        action='store',
        default='xml_inline_cfgs.c',
        type=str,
        help='Output C file to store inline XML',

    )

    args = parser.parse_args(sys.argv[1:])

    root_path = args.cfg_path
    c_code_output = args.out

    xmls2inline = collect_xmls(root_path)

    if len(xmls2inline) == 0:
        print('No XML files to inline')

    c_fprint = new_file_write_call(f'./{c_code_output}')

    files_num = gen_code(c_fprint, xmls2inline, root_path)

    print(f'{files_num} XML files is written to {c_code_output}')

