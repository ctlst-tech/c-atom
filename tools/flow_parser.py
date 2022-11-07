import xml.etree.ElementTree as ET
import fspeclib
from typing import *
from common import *


def FlowFunctionFromXml(name, spec_node):

    inputs = []
    outputs = []
    params = []

    for i in spec_node.find('inputs').findall('i'):
        inputs.append(ctlst.Input(name=i.get('alias'), title=i.get('title'), value_type='core.type.f64'))
    for o in spec_node.find('outputs').findall('o'):
        outputs.append(ctlst.Output(name=o.get('alias'), title=o.get('title'), value_type='core.type.f64'))
    for alias in spec_node.find('params').findall('alias'):
        mv = alias.get('mandatory')
        m = True
        if mv is not None:
            m = False if str.lower(alias.get('mandatory')) == 'no' or str.lower(alias.get('mandatory')) == 'false' \
                         or str.lower(alias.get('mandatory')) == '0' else True

        params.append(ctlst.Parameter(name=alias.get('alias'), title=alias.get('title'),
                                      value_type='core.type.64',
                                      mandatory=m,
                                      default=alias.get('default')
                                      )
                      )

    return ctlst.Function(name=name, title='xml_based', inputs=inputs, outputs=outputs, parameters=params, dynamic=True)


class InvocationTuple():
    def __init__(self, *, alias: str, value: str):
        self.alias = alias
        self.value = value


class InvocationFunction():
    def __init__(self, *,
                 name: str,
                 by_spec: str,
                 inputs: Optional[List[InvocationTuple]] = None,
                 parameters: Optional[List[InvocationTuple]] = None):
        self.name = name
        self.by_spec_alias = by_spec
        self.inputs = inputs
        self.parameters = parameters

        self.var_initial_params = f'{self.name}_initial_params'
        self.var_conn_spec = f'{self.name}_conn_spec'

        self.spec_ref = None


    @staticmethod
    def find(lst: List, alias: str):
        for e in lst:
            if e.alias == alias:
                return e
        return None

    def find_param(self, alias: str):
        return self.find(self.parameters, alias)

    def find_input(self, alias: str):
        return self.find(self.inputs, alias)

    def generate_initial_params(self, fprint):
        if self.has_parameters():
            fprint(f'static const func_param_t {self.var_initial_params}[] = {{')
            for alias in self.parameters:
                fprint(f'    {{.alias = \"{alias.alias}\", .value=\"{alias.value}\"}},')

            fprint(f'}};')
            fprint()

    def has_parameters(self):
        return len(self.parameters) > 0

    def has_inputs(self):
        return len(self.inputs) > 0

    def generate_conn_spec(self, fprint):
        if self.has_inputs():
            fprint(f'static const func_conn_spec_t {self.var_conn_spec}[] = {{')
            for inp in self.inputs:
                # fprint(f'    {{')
                # fprint(f'        .alias = \"{inp.alias}\",')
                # fprint(f'        .value = \"{inp.value}\",')
                # fprint(f'    }},')
                fprint(f'    {{ .alias = \"{inp.alias}\", .value = \"{inp.value}\"}},')
            fprint(f'}};')

    def generate_func_instance_init(self, fprint):
        handler_name = 'NULL' if (self.spec_ref is None) \
                                 or (self.spec_ref.generator is None) \
            else f'&{self.spec_ref.generator.handler_name}'
        params_init = f'{self.var_initial_params}' if self.has_parameters() else 'NULL'
        conn_spec = f'{self.var_conn_spec}' if self.has_inputs() else 'NULL'
        fprint(f'    {{')
        fprint(f'        .name = "{self.name}",')
        fprint(f'        .h = {handler_name},')
        fprint(f'        .initial_params = {params_init},')
        fprint(f'        .connect_spec = {conn_spec}')
        fprint(f'    }},')

def invocation_function_from_xml(f_node):
    inputs = []
    params = []
    for i in f_node.findall('in'):
        inputs.append(InvocationTuple(alias=i.get('alias'), value=i.text))

    for alias in f_node.findall('param'):
        params.append(InvocationTuple(alias=alias.get('alias'), value=alias.text))

    name = f_node.get('name')
    by_spec = f_node.get('by_spec')

    return InvocationFunction(name=name, by_spec=by_spec, inputs=inputs, parameters=params)

class OutputLink:
    def __init__(self, alias: str, src_path: str):
        self.alias = alias
        self.src_path = src_path

class FlowCfg():

    def func_list_process(self, pred):
        for f in self.inv_functions:
            if pred(f):
                return False

        return True

    def __init__(self, xml_path):
        root_tree = ET.parse(xml_path)
        self.name = root_tree.getroot().get('name')

        spec_node = root_tree.find('spec')
        self.spec = FlowFunctionFromXml(self.name, spec_node)
        self.inv_functions = []
        self.output_links = []

        self.var_function_inv_array = f'functions_{self.name}'

        for f in root_tree.find('functions').findall('f'):
            inv_f = invocation_function_from_xml(f)
            if not self.func_list_process(lambda _f : _f.name == inv_f.name):
                raise Exception(f'Name \'{inv_f.name}\' is repeated, change it')

            self.inv_functions.append(inv_f)

        for link in root_tree.find('link_outputs').findall('link'):
            self.output_links.append(OutputLink(link.get('alias'), link.text))


    def init_and_validate(self):
        #statuc_funcs_names = (f.name for f in ctlst.functions)
        err = 0

        # attach specifications
        valid_invocations = []

        for inv_func in self.inv_functions:
            inv_func.spec_ref = ctlst.find_function(inv_func.by_spec_alias)
            if inv_func.spec_ref is None:
                print(f'Function invocation {inv_func.name} refers to non existing specification {inv_func.by_spec_alias}')
                err += 1
            else:
                valid_invocations.append(inv_func)

        def mandatory_list(some_list):
            l = []
            for e in some_list:
                if e.mandatory:
                    l.append(e)

            return l

        for inv_func in valid_invocations:

            mandatory_params = mandatory_list(inv_func.spec_ref.parameters)
            mandatory_inputs = mandatory_list(inv_func.spec_ref.inputs)


            # attach parameters
            inv_params_aliases = []

            for inv_param in inv_func.parameters:
                inv_param.spec_ref = inv_func.spec_ref.find_param(inv_param.alias)
                if inv_param.spec_ref is None:
                    err += 1
                    print(f'Function invocation {inv_func.name} parameter {inv_param.alias} '
                           f'is not specified in {inv_func.by_spec_alias}')

                # check duplications of params
                if inv_param.alias in inv_params_aliases:
                    print(f'Function invocation {inv_func.name} parameter {inv_param.alias} '
                          f'is specified more than once')
                else:
                    inv_params_aliases.append(inv_param.alias)


            # check that mandatory params are set
            for spec_param in mandatory_params:
                if inv_func.find_input(spec_param.name) is None:
                    err += 1
                    print(f'Function invocation {inv_func.name} '
                          f'does not have a mandatory parameter value {spec_param.name}')



            # attach input ref
            for inv_input in inv_func.inputs:
                inv_input.spec_ref = inv_func.spec_ref.find_input(inv_input.alias)
                if inv_input.spec_ref is None:
                    err += 1
                    print(f'Function invocation {inv_func.name} input {inv_input.alias} '
                           f'is not specified in {inv_func.by_spec_alias}')

            # check that mandatory params are set
            for spec_inp in mandatory_inputs:
                if inv_func.find_input(spec_inp.name) is None:
                    err += 1
                    print(f'Function invocation {inv_func.name} '
                          f'does not have a mandatory input {spec_inp.name}')

            # TODO validate parameters right here


    def generate_call_batch(self, c_file_path):

        fprint = new_file_write_call(c_file_path)

        fprint(f'/* Generated file. Do not edit! */')
        fprint()
        fprint(f'#include "function.h"')
        fprint(f'#include "fspecs.h"')
        fprint()

        for inv_func in self.inv_functions:

            fprint()
            ref_name = 'N/A' if inv_func.spec_ref is None else inv_func.spec_ref.name
            fprint(f'/* Initializing structures for invication of "{ref_name}" named "{inv_func.name}" */')
            inv_func.generate_initial_params(fprint)
            inv_func.generate_conn_spec(fprint)


        fprint()
        fprint(f'/* Initializing flow "{self.name}" function invocations array */')
        fprint(f'function_inside_flow_t {self.var_function_inv_array}[] = {{')
        for inv_func in self.inv_functions:
            inv_func.generate_func_instance_init(fprint)
        fprint(f'    {{')
        fprint(f'        .h = NULL')
        fprint(f'    }}')
        fprint(f'}};')


import crawler

#flow_function = FunctionFromXml('flow_cfg.xml')

#ctlst.load(['core'])
#ctlst.load(['../f_spec/core'])



flow_cfg = FlowCfg('../tests/flow_cfg.xml')
flow_cfg.init_and_validate()

flow_cfg.generate_call_batch('../test.c')

pass

#https://docs.python.org/3/library/xml.etree.elementtree.html#module-xml.etree.ElementTree
