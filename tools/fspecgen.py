#!/usr/bin/env python3
import sys

import ctlst
import os.path
from typing import *
from common import *

override_implementation = False

src_files_dict = dict()

def create_generated_file(*, full_path: str, tag: str ):
    if tag != 'None':
        if not tag in src_files_dict:
            src_files_dict[tag] = []

        src_files_dict[tag].append(full_path)

    return open(full_path, 'w')

def wrap_if_required(expr, parent_expr):
    if type(expr) in [ctlst.ThisValue, int, float]:
        return expr
    return f'{expr}'


def generate_expression(expr, map_name):
    if isinstance(expr, ctlst.ParameterValue):
        return map_name(expr.name)
    elif type(expr) in [int, float]:
        return f'{expr}'
    elif type(expr) is ctlst.ExpressionBinaryOperation:
        lhs = wrap_if_required(generate_expression(expr.lhs, map_name), expr)
        rhs = wrap_if_required(generate_expression(expr.rhs, map_name), expr)
        return f'{lhs} {expr.operator} {rhs}'
    raise 'Unsupported expression type'


def add_prefix(name, prefix):
    return f'{prefix}{name}'


def replace_name(name, mapper):
    if name in mapper:
        return mapper[name]
    return name

class GeneratedType:
    def __init__(self, spec: ctlst.Type):
        self.spec = spec
        self.h_path = spec.get_common_h_filepath()
        self.rel_h_path = os.path.relpath(self.h_path)

    def generate(self):
        common_file = create_generated_file(full_path=self.h_path, tag='types')

        def fprint(*args, **kwargs):
            print(*args, **kwargs, file=common_file)

        type_name = self.spec.get_escaped_name()

        header_guard = f'ctlst_{type_name}_h'.upper()

        fprint(f'#ifndef {header_guard}')
        fprint(f'#define {header_guard}')
        fprint()

        fprint(f'// TODO: Declare type alias `{type_name}`')
        fprint(f'#error "Type alias `{type_name}` is not declared."')
        fprint(f'// typedef {type_name} void;')

        fprint()
        fprint(f'#endif // {header_guard}')

class GPackage:
    def __init__(self, pkg: ctlst.Package):
        self.package = pkg

    def process_types(self):
        for type_ in self.package.types:
            common_filename = type_.get_common_h_filepath()
            gt = GeneratedType(type_)

            if not os.path.exists(common_filename) or override_implementation:
                gt.generate()


    def process_constants(self):
        for constant in self.package.constants:
            common_filename = constant.get_common_h_filepath()

            header_file = create_generated_file(full_path=common_filename, tag='consts')

            constant_id = constant.get_escaped_name().upper()
            header_guard = f'ctlst_{constant_id}_h'.upper()

            def fprint(*args, **kwargs):
                print(*args, **kwargs, file=header_file)

            fprint('/**')
            fprint(' *  Automatically-generated file. Do not edit!')
            fprint(' */')
            fprint()

            fprint(f'#ifndef {header_guard}')
            fprint(f'#define {header_guard}')
            fprint()

            fprint(f'#define {constant_id} ({constant.value})')
            fprint()

            fprint(f'#endif // {header_guard}')


    def process_structures(self):
        for structure in self.package.structures:
            structure_name = structure.get_escaped_name()

            header_filename = structure.get_common_h_filepath()
            header_file = create_generated_file(full_path=header_filename, tag='structs')

            def fprint(*args, **kwargs):
                print(*args, **kwargs, file=header_file)

            header_guard = f'ctlst_{structure_name}_h'.upper()

            fprint(f'#ifndef {header_guard}')
            fprint(f'#define {header_guard}')
            fprint()

            dependency_types = structure.get_dependency_types()
            if len(dependency_types) > 0:
                fprint('/* Include declaration of dependency types */')
                includes = []
                for dependency_type in dependency_types:
                    includes.append(dependency_type.get_common_h_filename())
                includes.sort()
                for include in includes:
                    fprint(f'#include "{include}"')
                fprint()

            fprint(f'/**')
            fprint(f' * @brief {structure.title.en}')
            if structure.description is not None:
                fprint(f' * {structure.description.en}')
            fprint(f' */')
            fprint(f'typedef struct {structure_name}_')
            fprint(f'{{')

            for member in structure.members:
                fprint(f'    /**')
                fprint(f'     * @brief {member.title.en}')
                fprint(f'     */')
                fprint(f'    {member.value_type.get_c_type_name()} {member.name};')

                fprint()

            fprint(f'}} {structure_name}_t;')

            fprint()
            fprint(f'#endif // {header_guard}')


class GType:
    def __init__(self, *, typename: str):
        self.typename = typename

    def get_name(self):
        return self.typename

class GArgument:
    def __init__(self, *, type: GType, varname: str, ptr: bool = False, const: bool = False):
        self.type = type
        self.name = varname
        self.ptr = ptr
        self.const = const

    def get_declaration(self):
        ptr = '*' if self.ptr else ''
        const = 'const ' if self.const else ''
        return f'{const}{self.type.typename} {ptr}{self.name}'

    def get_symbol(self):
        return self.name

class GCallableFunction:
    def __init__(self, *, name: str, return_type: str, arguments: Optional[List[GArgument]]):
        self.arguments = arguments
        self.name = name
        self.return_type = return_type

    def symbol(self):
        return self.name

    def call(self, arguments: Optional[List[str]]):
        rv = self.symbol()
        if not arguments is None:
            args = ', '.join(arguments)
        else:
            args = ''
        rv += f'({args});'
        return rv

    def prototype(self):
        return f'{self.declaration()};'

    def declaration(self):
        rv = f'{self.return_type} {self.name}('
        arg_eol = ' '
        arg_tab = ''

        l = len(rv)
        for a in self.arguments:
            l += len(a.get_declaration()) + 2

        if l > 120:
            rv += '\n'
            arg_eol = '\n'
            arg_tab = '    '

        for a in self.arguments:
            comma = ',' if a != self.arguments[-1] else ''
            if a == self.arguments[-1] and arg_eol == ' ':
                arg_eol = ''
            rv += f'{arg_tab}{a.get_declaration()}{comma}{arg_eol}'
        rv += ')'
        return rv

    def dummy_implementation(self):
        rv  = f'{{\n'
        rv += f'    // TODO: Implement flow_exec method for `{self.name}`\n'
        rv += f'    #error "Function `{self.name}_exec` is not implemented."\n'
        rv += f'}}\n'
        return rv



class GeneratedFunction:

    def func_name_prefix(self):
        return self.spec.get_prefix()

    def __init__(self, func: ctlst.Function, processor):
        self.spec = func
        self.processor = processor
        self.spec.generator = self
        self.files = []
        self.cmake_src_list = []
        self.function_name = func.get_escaped_name()

        self.cmakelists_src_define = f'F_SPEC_{self.function_name.upper()}_SRC'
        self.cmakelists_lib_name = f'f_spec_{self.function_name.lower()}-static'

        self.spec_name = f'ctlst_{self.spec.get_prefix()}_spec'
        self.handler_name = f'ctlst_{self.spec.get_prefix()}_handler'
        self.calls_struct_name = f'ctlst_{self.spec.get_prefix()}_calls'
        self.generated_code_dir = 'g'

        # compute params function
        self.type_params = GType(typename=f'{func.get_prefix()}_params_t')
        self.type_params_flags = GType(typename=f'{func.get_prefix()}_params_flags_t')

        self.callable_compute_params = GCallableFunction(name=f'{self.func_name_prefix()}_compute_params',
                                                         return_type='void',
                                                         arguments=[GArgument(type=self.type_params, varname='old_p', ptr=True),
                                                                    GArgument(type=self.type_params, varname='new_p', ptr=True),
                                                                    GArgument(type=self.type_params_flags, varname='flags', ptr=True)
                                                                    ])

        exec_func_args = []

        # flow_update function
        self.type_inputs = GType(typename=f'{func.get_prefix()}_inputs_t')
        self.type_outputs = GType(typename=f'{func.get_prefix()}_outputs_t')
        self.type_params = GType(typename=f'{func.get_prefix()}_params_t')
        self.type_state = GType(typename=f'{func.get_prefix()}_state_t')
        self.type_injection = GType(typename=f'{func.get_prefix()}_injection_t')
        self.type_input_connect_spec = GType(typename='func_conn_spec_t')
        self.type_void = GType(typename='void')
        self.type_char = GType(typename='char')
        self.type_int = GType(typename='int')
        self.type_eswb_topic_descr = GType(typename='eswb_topic_descr_t')


        if func.has_inputs():
            exec_func_args.append(GArgument(type=self.type_inputs, varname='i', ptr=True, const=True))
        if func.has_outputs():
            exec_func_args.append(GArgument(type=self.type_outputs, varname='o', ptr=True))
        if func.has_parameters():
            exec_func_args.append(GArgument(type=self.type_params, varname='p', ptr=True, const=True))
        if func.has_state():
            exec_func_args.append(GArgument(type=self.type_state, varname='state', ptr=True))
        if func.has_injection():
            exec_func_args.append(GArgument(type=self.type_injection, varname='injection', ptr=True, const=True))

        self.callable_exec = GCallableFunction(name=f'{self.func_name_prefix()}_exec',
                                               return_type='void',
                                               arguments=exec_func_args)

        # interface calls
        self.type_interface = GType(typename=f'{func.get_prefix()}_interface_t')
        mounting_td = GArgument(type=self.type_eswb_topic_descr, varname='mounting_td')
        interface_calls_arg = GArgument(type=self.type_interface, varname='interface', ptr=True)
        func_name_str = GArgument(type=self.type_char, varname='func_name', ptr=True, const=True)
        connect_spec = GArgument(type=self.type_input_connect_spec, varname='conn_spec', ptr=True, const=True)

        self.callable_interface_inputs_init = GCallableFunction(name=f'{self.func_name_prefix()}_interface_inputs_init',
                                                                return_type='int',
                                                                arguments=[interface_calls_arg,
                                                                           connect_spec,
                                                                           mounting_td])

        self.callable_interface_inputs_update = GCallableFunction(name=f'{self.func_name_prefix()}_interface_inputs_update',
                                                                  return_type='fspec_rv_t', arguments=[interface_calls_arg])

        self.callable_interface_outputs_init = GCallableFunction(name=f'{self.func_name_prefix()}_interface_outputs_init',
                                                                 return_type='fspec_rv_t',
                                                                 arguments=[interface_calls_arg, connect_spec, mounting_td, func_name_str])

        self.callable_interface_outputs_update = GCallableFunction(name=f'{self.func_name_prefix()}_interface_outputs_update',
                                                                   return_type='fspec_rv_t', arguments=[interface_calls_arg])

        self.callable_interface_exec = GCallableFunction(name=f'{self.func_name_prefix()}_interface_update',
                                                           return_type='void', arguments=[interface_calls_arg])




        # set params func
        self.type_func_param = GType(typename='func_param_t')

        params = GArgument(type=self.type_params, varname='params', ptr=True)
        params_pairs = GArgument(type=self.type_func_param, varname='param_pairs', ptr=True, const=True)
        initial_call_flag = GArgument(type=self.type_int, varname='initial_call')

        self.callable_set_params = GCallableFunction(name=f'{self.func_name_prefix()}_set_params',
                                                     return_type='fspec_rv_t',
                                                     arguments=[params, params_pairs, initial_call_flag])

        # func calls
        data_handle = GArgument(type=self.type_void, varname='dh', ptr=True)

        self.callable_call_init_inputs = GCallableFunction(name=f'{self.func_name_prefix()}_call_init_inputs',
                                                      return_type='fspec_rv_t',
                                                      arguments=[data_handle, connect_spec, mounting_td])

        self.callable_call_init_outputs = GCallableFunction(name=f'{self.func_name_prefix()}_call_init_outputs',
                                                      return_type='fspec_rv_t',
                                                      arguments=[data_handle, connect_spec, mounting_td, func_name_str])

        self.callable_call_exec = GCallableFunction(name=f'{self.func_name_prefix()}_call_exec',
                                                    return_type='void',
                                                    arguments=[GArgument(type=self.type_void, varname='dh', ptr=True)])

        self.callable_call_set_params = GCallableFunction(name=f'{self.func_name_prefix()}_call_set_params',
                                                      return_type='fspec_rv_t',
                                                      arguments=[data_handle, params_pairs, initial_call_flag])


    def decl_params(self, func, fprint):
        fprint(f'/**')
        fprint(f' * @brief Parameters of `{func.name}` function')
        fprint(f' */')
        fprint(f'typedef struct {func.get_prefix()}_params_\n{{')
        for parameter in func.parameters:
            value_type = parameter.value_type
            if type(value_type) is not ctlst.VectorOf:
                fprint(f'    /**')
                fprint(f'     * @brief {parameter.title.en}')
                fprint(f'     */')
                fprint(f'    {value_type.get_c_type_name()} {parameter.name};')
                fprint()
        fprint(f'}} {func.get_prefix()}_params_t;')
        fprint()

    def decl_inputs(self, func, fprint):
        fprint(f'/**')
        fprint(f' * @brief Inputs of `{func.name}` function')
        fprint(f' */')
        fprint(f'typedef struct {func.get_prefix()}_inputs_\n{{')
        for inp in func.inputs:
            value_type = inp.value_type
            fprint(f'    /** @brief {inp.title.en}*/')
            fprint(f'    {value_type.get_c_type_name()} {inp.name};')
        fprint()
        for inp in func.inputs:
            if not inp.mandatory:
                fprint(f'    uint32_t opt_in_actvtd_{inp.name}:1;')

        fprint(f'}} {func.get_prefix()}_inputs_t;')
        fprint()

    def decl_outputs(self, func, fprint):
        fprint(f'/**')
        fprint(f' * @brief Outputs of `{func.name}` function')
        fprint(f' */')
        fprint(f'typedef struct {func.get_prefix()}_outputs_\n{{')
        for output in func.outputs:
            value_type = output.value_type
            fprint(f'    /**')
            fprint(f'     * @brief {output.title.en}')
            fprint(f'     */')
            fprint(f'    {value_type.get_c_type_name()} {output.name};')
            fprint()
        fprint(f'}} {func.get_prefix()}_outputs_t;')
        fprint()

    def decl_state(self, func, fprint):
        fprint(f'/**')
        fprint(f' * @brief State variables of `{func.name}` function')
        fprint(f' */')
        fprint(f'typedef struct {func.get_prefix()}_state_\n{{')
        for variable in func.state:
            value_type = variable.value_type
            fprint(f'    /**')
            fprint(f'     * @brief {variable.title.en}')
            fprint(f'     */')
            fprint(f'    {value_type.get_c_type_name()} {variable.name};')
            fprint()
        fprint(f'}} {func.get_prefix()}_state_t;')
        fprint()

    def decl_injection(self, func, fprint):
        fprint(f'/**')
        fprint(f' * @brief Injections for `{func.name}` function')
        fprint(f' */')
        fprint(f'typedef struct {func.get_prefix()}_injection_\n{{')

        if func.injection.timedelta:
            fprint(f'    /**')
            fprint(f'     * @brief Time delta from previous iteration (measured in seconds)')
            fprint(f'     */')
            fprint(f'    {self.processor.find_type("core.type.f64").get_c_type_name()} dt;')
            fprint()

        if func.injection.timestamp:
            fprint(f'    /**')
            fprint(f'     * @brief Current timestamp (measured in microseconds)')
            fprint(f'     */')
            fprint(f'    {self.processor.find_type("core.type.u64").get_c_type_name()} timestamp;')
            fprint()

        fprint(f'}} {func.get_prefix()}_injection_t;')
        fprint()

    def decl_interface_structs(self, func, fprint):

        if func.has_inputs() or func.has_outputs():
            fprint(f'typedef struct {self.function_name}_eswb_descriptors_')
            fprint(f'{{')

            if func.has_inputs:
                for input in func.inputs:
                    fprint(f'    eswb_topic_descr_t in_{input.name};')

            if func.has_outputs():
                fprint(f'    eswb_topic_descr_t out_all;')

            fprint(f'}} {self.function_name}_eswb_descriptors_t;')
            fprint()

        fprint(f'typedef struct {self.function_name}_interface_')
        fprint(f'{{')

        #fprint(f'    char *name;')
        #fprint(f'    eswb_topic_descr_t mnt_pnt_td;')

        if func.has_inputs():
            fprint(f'    {func.get_prefix()}_inputs_t i;')

        if func.has_outputs():
            fprint(f'    {func.get_prefix()}_outputs_t o;')

        if func.has_parameters():
            fprint(f'    {func.get_prefix()}_params_t p;')

        if func.has_state():
            fprint(f'    {func.get_prefix()}_state_t state;')

        if func.has_injection():
            fprint(f'    {func.get_prefix()}_injection_t injection;')
            if func.injection.timedelta:
                fprint(f'    struct timespec prev_exec_time;')
                fprint(f'    int prev_exec_time_inited;')

        if func.has_inputs() or func.has_outputs():
            fprint(f'    {self.function_name}_eswb_descriptors_t eswb_descriptors;')

        fprint(f'}} {self.function_name}_interface_t;')
        fprint()


    def decl_params_flags(self, func, fprint):
        fprint(f'/**')
        fprint(f' * @brief Parameter flags of `{func.name}` function')
        fprint(f' */')
        fprint(f'typedef struct {func.get_prefix()}_params_flags_\n{{')
        for parameter in func.parameters:
            fprint(f'    uint64_t changed_{parameter.name}:1;')
        fprint(f'}} {func.get_prefix()}_params_flags_t;')
        fprint()

    def get_types_includes(self):
        dependency_types = self.spec.get_dependency_types()
        includes = []
        if len(dependency_types) > 0:
            for dependency_type in dependency_types:
                if type(dependency_type) is not ctlst.VectorOf:
                    includes.append(dependency_type.get_common_h_filename())
            includes.sort()

        return includes

    def decl_header(self, hdr_file_name):
        fprint = new_file_write_call(hdr_file_name)
        f_spec = self.spec

        header_guard = f'ctlst_{self.function_name}_h'.upper()

        fprint('/**')
        fprint(' *  Automatically-generated file. Do not edit!')
        fprint(' */')

        fprint()

        fprint(f'#ifndef {header_guard}')
        fprint(f'#define {header_guard}')
        fprint()

        fprint(f'#include <stdint.h>')
        fprint(f'#include <eswb/types.h>')
        fprint()

        fprint(f'#include "function.h"')
        fprint()

        includes = self.get_types_includes()

        if len(includes) > 0:
            fprint('/* Include declaration of dependency types */')
            for include in includes:
                fprint(f'#include "{include}"')
            fprint()

        if f_spec.has_parameters():
            self.decl_params(f_spec, fprint)

        if f_spec.has_inputs():
            self.decl_inputs(f_spec, fprint)

        if f_spec.has_outputs():
            self.decl_outputs(f_spec, fprint)

        if f_spec.has_state():
            self.decl_state(f_spec, fprint)

        if f_spec.has_injection():
            self.decl_injection(f_spec, fprint)

        if f_spec.has_parameters() or f_spec.has_state():
            self.decl_params_flags(f_spec, fprint)

        self.decl_interface_structs(f_spec, fprint)

        if f_spec.has_parameters():
            fprint(f'{self.callable_set_params.prototype()}')
            fprint()

        if f_spec.has_compute_parameters():
            fprint(f'{self.callable_compute_params.prototype()}')
            fprint()

        fprint(f'{self.callable_exec.prototype()}')
        fprint()

        fprint(f'#endif // {header_guard}')


    def decl_impl_set_params(self, set_params_filename):

        f_spec = self.spec
        fprint = new_file_write_call(set_params_filename)

        pps = self.callable_set_params.arguments[-2].name
        ifs = self.callable_set_params.arguments[-1].name

        fprint(f'#include "{f_spec.get_name()}.h"')
        fprint()

        fprint('#include <string.h>')
        fprint()

        fprint(f'#include "error.h"')
        fprint()

        fprint(f'{self.callable_set_params.declaration()}')
        fprint(f'{{', )

        fprint(f'    // Get parameters', )
        fprint(f'    {f_spec.get_prefix()}_params_t p = *params;', )
        fprint()
        fprint(f'    {self.type_params_flags.get_name()} flags;')
        fprint(f'    memset(&flags, 0, sizeof(flags));')
        fprint()
        fprint(f'    int violation_count = 0;', )
        fprint()

        fprint(f'    // Parse parameters', )
        fprint(f'    for (int i = 0; {pps}[i].alias != NULL; i++) {{', )
        fprint('        ', end='', )
        for parameter in f_spec.parameters:
            if parameter != f_spec.parameters[0]:
                fprint(' else ', end='', )
            fprint(f'if (strcmp({pps}[i].alias, "{parameter.name}") == 0) {{', )

            if parameter.value_type.name == 'core.type.f64':
                fprint(f'            if (sscanf({pps}[i].value, "%lf", &p.{parameter.name}) < 1) {{')
                fprint(f'                error("Parameter \'%s\' format error (%s)", {pps}[i].alias, {pps}[i].value);')
                fprint(f'                violation_count++;')
                fprint(f'            }}')
            else:
                fprint(f'            // TODO: Parse into p.{parameter.name}')

            fprint(f'            flags.changed_{parameter.name} = 1;', )
            # params.Kp = atof(param_str[i].value);
            fprint('        }', end='', )
            pass
        fprint(' else {', )
        fprint(f'            error("failed unsupported parameter \'%s\'", {pps}[i].alias);', )
        fprint(f'            violation_count ++;', )
        fprint('        }', )
        fprint('    }', )
        fprint()

        fprint(f'    if ({ifs}) {{', )
        if f_spec.has_mandatory_parameters():
            fprint('        // Check all mandatory parameters are set', )
            for parameter in f_spec.parameters:
                if not parameter.mandatory:
                    continue
                fprint(f'        if (!flags.changed_{parameter.name}) {{', )
                fprint(f'            error("failed missed mandatory parameter \'{parameter.name}\'");', )
                fprint(f'            violation_count ++;', )
                fprint(f'        }}', )

        if f_spec.has_non_mandatory_parameters():
            if f_spec.has_mandatory_parameters():
                fprint()
            fprint('        // Set default value for non mandatory parameters which are not set', )
            for parameter in f_spec.parameters:
                if parameter.mandatory or type(parameter) == ctlst.ComputedParameter:
                    continue
                fprint(f'        if (!flags.changed_{parameter.name}) {{', )
                fprint(f'            p.{parameter.name} = {parameter.default};', )
                fprint(f'            flags.changed_{parameter.name} = 1;', )
                fprint(f'        }}', )
        fprint('    }', )
        fprint()

        fprint(f'    if (violation_count > 0) {{', )
        fprint(f'        return fspec_rv_inval_param;', )
        fprint(f'    }}', )
        fprint()

        expressions = []

        for parameter in f_spec.parameters:
            this_value_replacer_lambda = lambda x: replace_name(x, {'__this_value__': parameter.name})
            prefix_adder_lambda = lambda x: add_prefix(this_value_replacer_lambda(x), 'p.')
            for constraint in parameter.constraints:
                expression = generate_expression(constraint, prefix_adder_lambda)
                error_expression = generate_expression(constraint, this_value_replacer_lambda)
                expressions.append((expression, error_expression))

        for constraint in f_spec.mutual_parameter_constraints:
            prefix_adder_lambda = lambda x: add_prefix(x, 'p.')
            nothing_lambda = lambda x: x
            expression = generate_expression(constraint, prefix_adder_lambda)
            error_expression = generate_expression(constraint, nothing_lambda)
            expressions.append((expression, error_expression))

        expression_statements = []
        for expression in expressions:
            expression_statement = f'if (!({expression[0]})) {{\n' \
                                   f'        error(\"failed constraint ({expression[1]})\");\n' \
                                   '        return fspec_rv_inval_param;\n' \
                                   '    }'
            expression_statements.append(expression_statement)

        if len(expression_statements) > 0:
            fprint(f'    // Validate parameters', )
            fprint(f'    {" else ".join(expression_statements)}', )
            fprint()

        if f_spec.has_compute_parameters():
            fprint(f'    // Compute parameters', )
            local_arguments = ['params', '&p', '&flags']
            fprint(f'    {f_spec.get_prefix()}_compute_params({", ".join(local_arguments)});', )
            fprint()

        fprint(f'    // Set parameters', )
        fprint(f'    *params = p;', )
        fprint()

        fprint(f'    return fspec_rv_ok;', )

        fprint(f'}}', )

    def decl_impl_exec(self, upd_filename):
        f_spec = self.spec

        fprint = new_file_write_call(upd_filename)

        fprint(f'#include "{f_spec.get_name()}.h"', )
        fprint()

        fprint(f'{self.callable_exec.declaration()}')
        fprint(f'{self.callable_exec.dummy_implementation()}')


    def decl_impl_compute_params(self, impl_compute_params_filename):
        # Generate implementation for configure state
        f_spec = self.spec
        fprint = new_file_write_call(impl_compute_params_filename)

        fprint(f'#include "{f_spec.get_name()}.h"')
        fprint()

        fprint(f'#include "error.h"')
        fprint()

        fprint(f'{self.callable_compute_params.declaration()}')
        fprint(f'{self.callable_compute_params.dummy_implementation()}')

    def decl_spec_src(self, spec_source_filename):
        # Generate spec source file
        f_spec = self.spec
        fprint = new_file_write_call(spec_source_filename)

        fprint('/**')
        fprint(' *  Automatically-generated file. Do not edit!')
        fprint(' */')

        fprint()

        fprint(f'#include "{f_spec.get_name()}.h"')
        fprint(f'#include <eswb/types.h>')
        fprint()

        for parameter in f_spec.parameters:
            fprint(f'static const param_spec_t param_{parameter.name} = {{')
            fprint(f'    .name = "{parameter.name}",')
            fprint(f'    .default_value = "0.0",')
            fprint(f'    .annotation = "{parameter.title.en}",')
            fprint(f'    .flags = 0')
            fprint(f'}};')
            fprint()

        fprint(f'static const param_spec_t *params[{len(f_spec.parameters) + 1}] = {{')
        for parameter in f_spec.parameters:
            fprint(f'    &param_{parameter.name},')
        fprint(f'    NULL')
        fprint(f'}};')
        fprint()

        for input in f_spec.inputs:
            fprint(f'static const input_spec_t i_{input.name} = {{')
            fprint(f'    .name = "{input.name}",')
            fprint(f'    .annotation = "{input.title.en}",')
            fprint(f'    .flags = 0')
            fprint(f'}};')
            fprint()

        fprint(f'static const input_spec_t *inputs[{len(f_spec.inputs) + 1}] = {{')
        for input in f_spec.inputs:
            fprint(f'    &i_{input.name},')
        fprint(f'    NULL')
        fprint(f'}};')
        fprint()

        for output in f_spec.outputs:
            fprint(f'static const output_spec_t o_{output.name} = {{')
            fprint(f'    .name = "{output.name}",')
            fprint(f'    .annotation = "{output.title.en}",')
            fprint(f'    .flags = 0')
            fprint(f'}};')
            fprint()

        fprint(f'static const output_spec_t *outputs[{len(f_spec.outputs) + 1}] = {{')
        for output in f_spec.outputs:
            fprint(f'    &o_{output.name},')
        fprint(f'    NULL')
        fprint(f'}};')
        fprint()

        if self.spec.has_inputs():
            fprint(f'{self.callable_call_init_inputs.prototype()}')

        if self.spec.has_outputs():
            fprint(f'{self.callable_call_init_outputs.prototype()}')

        fprint(f'{self.callable_call_exec.prototype()}')

        if f_spec.has_parameters():
            fprint(f'{self.callable_call_set_params.prototype()}')
            fprint()

        fprint(f'const function_spec_t {self.spec_name} = {{')
        fprint(f'    .name = "{f_spec.name}",')
        fprint(f'    .annotation = "{f_spec.title.en}",')
        fprint(f'    .inputs = inputs,')
        fprint(f'    .outputs = outputs,')
        fprint(f'    .params = params')
        fprint(f'}};')
        fprint()

        fprint(f'const function_calls_t {self.calls_struct_name} = {{')
        fprint(f'    .interface_handle_size = sizeof({self.type_interface.get_name()}),')
        fprint(f'    .init = NULL,')
        init_in_sym = self.callable_call_init_inputs.symbol() if self.spec.has_inputs() else 'NULL'
        init_out_sym = self.callable_call_init_outputs.symbol() if self.spec.has_outputs() else 'NULL'
        fprint(f'    .init_inputs = {init_in_sym},')
        fprint(f'    .init_outputs = {init_out_sym},')
        fprint(f'    .exec = {self.callable_call_exec.symbol()},')
        sp = self.callable_call_set_params.symbol() if f_spec.has_parameters() else 'NULL'
        fprint(f'    .set_params = {sp}')
        fprint(f'}};')
        fprint()

        fprint(f'const function_handler_t {self.handler_name} = {{')
        fprint(f'    .spec = &{self.spec_name},')
        fprint(f'    .calls = &{self.calls_struct_name},')
        fprint(f'    .extension_handler = NULL')
        fprint(f'}};')

    def decl_interface_src(self, interface_source_filename):
        # Generate interface source file
        f_func = self.spec
        fprint = new_file_write_call(interface_source_filename)

        fprint('/**')
        fprint(' *  Automatically-generated file. Do not edit!')
        fprint(' */')
        fprint()

        fprint(f'#include "{self.function_name}.h"')
        fprint()

        fprint(f'#include "error.h"')
        fprint(f'#include <eswb/api.h>')
        fprint(f'#include <eswb/topic_proclaiming_tree.h>')
        fprint(f'#include <eswb/errors.h>')

        fprint()

        def impl_inputs_init():
            fprint(f'{self.callable_interface_inputs_init.declaration()}')
            fprint(f'{{')
            fprint(f'    eswb_rv_t rv;')
            fprint(f'    int errcnt_no_topic = 0;')
            fprint(f'    int errcnt_no_input = 0;')

            def topic_path(i):
                return f'topic_path_in_{i.name}'

            for inp in f_func.inputs:
                fprint(f'    const char *{topic_path(inp)} = fspec_find_path2connect(conn_spec,\"{inp.name}\");')

            fprint()

            for inp in f_func.inputs:
                mand = 'mandatory' if inp.mandatory else 'optional'
                fprint(f'    // Connecting {mand} input \"{inp.name}\"')
                fprint(f'    if ({topic_path(inp)} != NULL) {{')
                if not inp.mandatory:
                    fprint(f'        interface->i.opt_in_actvtd_{inp.name} = 1;')
                fprint(f'        rv = eswb_connect_nested(mounting_td, {topic_path(inp)}, &interface->eswb_descriptors.in_{inp.name});')
                fprint(f'        if(rv != eswb_e_ok) {{')
                fprint(f'            error("failed connect input \\"{inp.name}\\" to topic \\"%s\\": %s", {topic_path(inp)}, eswb_strerror(rv));')
                fprint(f'            errcnt_no_topic++;')
                fprint(f'        }}')
                fprint(f'    }}', end='')
                if inp.mandatory:
                    fprint(f' else {{')
                    fprint(f'        error("mandatory input \\"{inp.name}\\" is not speicifed");')
                    fprint(f'        errcnt_no_input++;')
                    fprint(f'    }}')
                else:
                    fprint()
                fprint()

            fprint(f'    if ((errcnt_no_input > 0) || (errcnt_no_topic > 0)) {{')
            fprint(f'        if (errcnt_no_input > errcnt_no_topic) {{')
            fprint(f'            return fspec_rv_no_input;')
            fprint(f'        }} else {{')
            fprint(f'            return fspec_rv_no_topic;')
            fprint(f'        }}')
            fprint(f'    }}')
            fprint(f'    return fspec_rv_ok;')
            fprint(f'}}')
            fprint()


        def impl_inputs_update():
            fprint(f'{self.callable_interface_inputs_update.declaration()}')
            fprint(f'{{')
            fprint(f'    eswb_rv_t rv;')
            fprint()

            for input in f_func.inputs:
                # is_first = (input == f_func.inputs[0])
                # getter = 'eswb_read' if (not is_first) else "eswb_get_update"
                getter = 'eswb_read'
                fprint(f'    rv = {getter}(interface->eswb_descriptors.in_{input.name}, &interface->i.{input.name});')
                fprint(f'    if(rv != eswb_e_ok) {{')
                fprint(f'        return 1;')
                fprint(f'    }}')
                fprint()
            fprint(f'    return 0;')
            fprint(f'}}')
            fprint()

        if f_func.has_inputs():
            impl_inputs_init()
            impl_inputs_update()


        def impl_outputs_init():
            fprint(f'{self.callable_interface_outputs_init.declaration()}')
            fprint(f'{{')
            fprint(f'    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, {len(f_func.outputs) + 1});')
            fprint(f'    {self.type_outputs.get_name()} out;')
            fprint(f'    eswb_rv_t rv;')
            # fprint(f'    topic_proclaiming_tree_t *topic;')
            fprint()
            fprint(f'    topic_proclaiming_tree_t *rt = usr_topic_set_struct(cntx, out, '
                   f'{self.callable_interface_outputs_init.arguments[-1].get_symbol()});')
            fprint()

            for output in f_func.outputs:
                fprint(f'    usr_topic_add_struct_child(cntx, rt, {self.type_outputs.get_name()}, {output.name}, "{output.name}", tt_double);')
                # fprint(f'    if (topic == NULL) {{')
                # fprint(f'        return 1;')
                # fprint(f'    }}')
                fprint()
            fprint(f'    rv = eswb_proclaim_tree(mounting_td, rt, cntx->t_num, &interface->eswb_descriptors.out_all);')
            fprint(f'    if (rv != eswb_e_ok) {{')
            fprint(f'        return fspec_rv_publish_err;')
            fprint(f'    }}')
            fprint()
            fprint(f'    return fspec_rv_ok;')
            fprint(f'}}')
            fprint()

        def impl_outputs_update():

            fprint(f'{self.callable_interface_outputs_update.declaration()}')
            fprint(f'{{')
            fprint(f'    eswb_rv_t rv;')
            fprint()
            fprint(f'    rv = eswb_update_topic(interface->eswb_descriptors.out_all, &interface->o);')
            fprint(f'    if (rv != eswb_e_ok) {{')
            fprint(f'        return 1;')
            fprint(f'    }}')
            fprint()
            fprint(f'    return 0;')
            fprint(f'}}')
            fprint()

        if f_func.has_outputs():
            impl_outputs_init()
            impl_outputs_update()


        def impl_interface_exec():
            fprint(f'{self.callable_interface_exec.declaration()}')
            fprint(f'{{')

            if f_func.has_inputs():
                fcall = self.callable_interface_inputs_update.call(['interface'])
                fprint(f'    {fcall}')

            # update_curr_time = False
            if f_func.has_injection():
                if f_func.injection.timedelta:
                    # update_curr_time = True
                    fprint('    if (interface->prev_exec_time_inited) {')
                    fprint('        function_getdeltatime(ft_monotonic, &interface->prev_exec_time, dtp_update_prev, &interface->injection.dt);')
                    fprint('    } else {')
                    fprint('        function_gettime(ft_monotonic, &interface->prev_exec_time);')
                    fprint('        interface->prev_exec_time_inited = -1;')
                    fprint('        interface->injection.dt = 0;')
                    fprint('    }')
                    fprint()

            arguments = []
            if f_func.has_inputs():
                arguments.append('&interface->i')
            if f_func.has_outputs():
                arguments.append('&interface->o')
            if f_func.has_parameters():
                arguments.append('&interface->p')
            if f_func.has_state():
                arguments.append('&interface->state')
            if f_func.has_injection():
                arguments.append('&interface->injection')
            fprint(f'    {self.callable_exec.call(arguments)}')

            if f_func.has_outputs():
                fcall = self.callable_interface_outputs_update.call(['interface'])
                fprint(f'    {fcall}')

            # if update_curr_time:
            #     fprint()
            #     fprint('    interface->prev_update_time = curr_time;')


            fprint(f'}}')
            fprint()

        impl_interface_exec()

        # fspec calls
        ## init
        def gen_wrapper(wrap, wrapped, do_ret=True, insertion='', arg_list=['interface']):
            fprint(f'{wrap.declaration()}')
            fprint(f'{{')
            #fprint(f'fspec_rv_t rv;')
            if len(insertion) > 0:
                fprint(f'{insertion}')
            fprint(f'    {self.type_interface.get_name()} *interface = ({self.type_interface.get_name()}*) dh;')
            ret = 'return ' if do_ret else ''
            fprint(f'    {ret}{wrapped.call(arg_list)}')
            fprint(f'}}')
            fprint()

        if (self.spec.has_parameters()):
            gen_wrapper(self.callable_call_set_params, self.callable_set_params,
                        arg_list=['&interface->p', self.callable_call_set_params.arguments[-2].name,
                                  self.callable_call_set_params.arguments[-1].name]
                        )

        if (self.spec.has_inputs()):
            gen_wrapper(self.callable_call_init_inputs, self.callable_interface_inputs_init,
                        arg_list=['interface', 'conn_spec', 'mounting_td'])


        if (self.spec.has_outputs()):
            gen_wrapper(self.callable_call_init_outputs, self.callable_interface_outputs_init,
                        arg_list=['interface', 'conn_spec', 'mounting_td', 'func_name'])

        ## flow_update
        gen_wrapper(self.callable_call_exec, self.callable_interface_exec, do_ret=False)


    def generate_cmkelists(self, cmakelists_path, custom):
        fprint = new_file_write_call(cmakelists_path)

        if not custom:
            fprint(f'#')
            fprint(f'# Generated file Do not edit')
            fprint(f'# If need custom CMakeLists.txt set custom_cmakefile=True in {self.spec.pkg_rel_declaration_path}')
            fprint(f'#')

        fprint(f'cmake_minimum_required(VERSION 3.6)')

        fprint(f'set({self.cmakelists_src_define}')
        for src_file in self.cmake_src_list:
            fprint(f'    {src_file}')
        fprint(f'    )')
        fprint()

        fprint()
        fprint(f'add_library({self.cmakelists_lib_name} STATIC ${{{self.cmakelists_src_define}}})')
        fprint()
        fprint(f'target_include_directories({self.cmakelists_lib_name} PRIVATE {self.generated_code_dir})')
        fprint(f'target_include_directories({self.cmakelists_lib_name} PRIVATE ../../../include)')
        fprint(f'target_include_directories({self.cmakelists_lib_name} '
               f'PRIVATE ../../../../eswb/src/lib/include/public)')
        fprint(f'target_include_directories({self.cmakelists_lib_name} PUBLIC ')
        for t in self.spec.get_dependency_types():
            fprint(f'    {os.path.relpath(t.directory, start=self.spec.directory)}')
        fprint(')')

        fprint(f'target_link_libraries({self.cmakelists_lib_name} PUBLIC eswb-if)')

    def generate_code(self, as_static_lib = False):

        # user defined (generated once)
        fn_exec_c = f'{self.function_name}_exec.c'
        fn_compute_params_c = f'{self.function_name}_compute_params.c'

        # generated
        fn_spec_c = f'{self.generated_code_dir}/{self.function_name}_spec.c'
        fn_set_params_c = f'{self.generated_code_dir}/{self.function_name}_set_params.c'
        fn_interface_c = f'{self.generated_code_dir}/{self.function_name}_interface.c'
        fn_h = f'{self.generated_code_dir}/{self.function_name}.h'

        full_gen_path = f'{self.spec.directory}/{self.generated_code_dir}'
        if not os.path.exists(full_gen_path):
            os.mkdir(full_gen_path)

        self.decl_header(f'{self.spec.directory}/{fn_h}')
        self.cmake_src_list.append(fn_h)

        gitgnore_path = f'{self.spec.directory}/.gitignore'
        if not os.path.exists(gitgnore_path):
            fprint = new_file_write_call(gitgnore_path)
            fprint(f'{self.generated_code_dir}')

        if self.spec.has_parameters():
            impl_set_params_filename = f'{self.spec.directory}/{fn_set_params_c}'
            self.decl_impl_set_params(impl_set_params_filename)
            self.cmake_src_list.append(fn_set_params_c)

        implementation_exec_filename = f'{self.spec.directory}/{fn_exec_c}'
        if not os.path.exists(implementation_exec_filename) or override_implementation:
            self.decl_impl_exec(implementation_exec_filename)
        self.cmake_src_list.append(fn_exec_c)

        if self.spec.has_compute_parameters():
            impl_compute_params_filename = f'{self.spec.directory}/{fn_compute_params_c}'
            if not os.path.exists(impl_compute_params_filename) or override_implementation:
                self.decl_impl_compute_params(impl_compute_params_filename)
            self.cmake_src_list.append(fn_compute_params_c)

        spec_source_filename = f'{self.spec.directory}/{fn_spec_c}'
        self.decl_spec_src(spec_source_filename)
        self.cmake_src_list.append(fn_spec_c)

        interface_source_filename = f'{self.spec.directory}/{fn_interface_c}'
        self.decl_interface_src(interface_source_filename)
        self.cmake_src_list.append(fn_interface_c)

        cmakelists_path = f'{self.spec.directory}/CMakeLists.txt'

        cmakefile_override_implementation = True

        if not self.spec.custom_cmakefile or not os.path.exists(cmakelists_path) or cmakefile_override_implementation:
            self.generate_cmkelists(cmakelists_path, self.spec.custom_cmakefile)

class FuncProcessor:
    def __init__(self, package: ctlst.Package, context: List[ctlst.Package], process_context=False):
        self.generated_funcs_list = []
        self.spec_header_filename = 'fspecs.h'

        self.package = package
        self.context = context

        if process_context:
            for pkg in context:
                for function in pkg.functions:
                    f = GeneratedFunction(function, self)
                    self.generated_funcs_list.append(f)
        else:
            for function in package.functions:
                f = GeneratedFunction(function, self)
                self.generated_funcs_list.append(f)

        self.fspec_registry_c_filename = f'{self.package.name}_registry.c'

    def find_type(self, alias):
        return ctlst.find_type_in_context(alias, self.context)

    def register_src_file(self, func_name):
        #tag='func_src_' + self.current_function_file_tag
        pass

    def generate_externs_declarations(self, fprint):
        fprint(f'#include "function.h"')
        fprint()

        for f in self.generated_funcs_list:
            fprint(f'extern const function_handler_t {f.handler_name};')

        fprint()

    def generate_fspecs_header(self, path):
        # Generate spec header

        fprint = new_file_write_call(f'{path}/{self.spec_header_filename}')

        header_guard = f'ctlst_fspec_h'.upper()

        fprint('/**')
        fprint(' *  Automatically-generated file. Do not edit!')
        fprint(' */')

        fprint()

        fprint(f'#ifndef {header_guard}')
        fprint(f'#define {header_guard}')
        fprint()

        self.generate_externs_declarations(fprint)

        fprint(f'#endif // {header_guard}')

    def generate_fspecs_registry_c(self, path):
        # Generate spec header

        fprint = new_file_write_call(f'{path}/{self.fspec_registry_c_filename}')

        fprint('/**')
        fprint(' *  Automatically-generated file. Do not edit!')
        fprint(' */')

        fprint()

        fprint(f'#include <stdlib.h>')
        fprint(f'#include <string.h>')
        fprint(f'#include "function.h"')
        # fprint(f'#include "{self.spec_header_filename}"')
        fprint()
        fprint()
        self.generate_externs_declarations(fprint)
        fprint()
        fprint()

        array_name = 'functions_spec_array'

        fprint(f'static const function_handler_t *{array_name}[] = {{')
        for f in self.generated_funcs_list:
            fprint(f'    &{f.handler_name},')
        fprint(f'    NULL')
        fprint(f'}};')
        fprint()
        fprint()
        fprint(f'const function_handler_t* fspec_find_handler(const char * spec_name) {{')
        fprint(f'    function_handler_t * rv;')
        fprint(f'    for (int i = 0; {array_name}[i] != NULL; i++) {{')
        fprint(f'        if (strcmp({array_name}[i]->spec->name, spec_name) == 0) {{')
        fprint(f'            return functions_spec_array[i];')
        fprint(f'        }}')
        fprint(f'    }}')
        fprint(f'    return NULL;')
        fprint(f'}}')

    def generate_bbxml(self, fpath):
        fprint = new_file_write_call(f'{fpath}')

        def bb2xml(fprint, fs):
            fprint(f'    <bblock type="{fs.name}" title="{fs.title}" group="{fs.namespace}">')
            fprint(f'        <description>{fs.description}</description>')

            fprint(f'        <inputs>')
            for i in fs.inputs:
                fprint(f'            <input id="{i.name}" required="{i.mandatory}" title="{i.title}" type="{i.value_type.name}">')
                if i.description is not None:
                    fprint(f'                {i.description}')
                fprint(f'            </input>')
            fprint(f'        </inputs>')

            fprint(f'        <outputs>')
            for o in fs.outputs:
                fprint(f'            <output id="{o.name}" title="{o.title}" type="{o.value_type.name}">')
                if o.description:
                    fprint(f'                {o.description}')
                fprint(f'            </output>')
            fprint(f'        </outputs>')
            fprint(f'        <params>')

            for prm in fs.parameters:
                fprint(f'            <param id="{prm.name}" title="{prm.title}" type="{prm.value_type.name}">')
                if prm.description:
                    fprint(f'                {prm.description}')
                fprint(f'            </param>')
            fprint(f'        </params>')
            fprint(f'    </bblock>')

        fprint('<BuildingBlocks_root>')

        for f in self.generated_funcs_list:
            bb2xml(fprint, f.spec)

        fprint('</BuildingBlocks_root>')

    def generate_cmake_lists(self, fpath, has_registry=True):
        fprint = new_file_write_call(f'{fpath}/CMakeLists.txt')

        fprint('# ')
        fprint('# Autogenerated file. Do not edit')
        fprint('# ')

        fprint('cmake_minimum_required(VERSION 3.6)')
        # fprint()
        # fprint('project(f_spec)')
        # fprint()

        for gf in self.generated_funcs_list:
            fprint(f'add_subdirectory({gf.spec.pkg_rel_directory})')

        fspec_src_name = f'FSPEC_{self.package.name.upper()}_REGISTRY_SRC'
        fspec_registry_target = f'fspec-{self.package.name.lower()}-registry'

        if has_registry:
            fprint(f'set({fspec_src_name} ')
            fprint(f'      {self.fspec_registry_c_filename}')
            fprint('    )')

        fspec_interface_target = f'fspec-{self.package.name.lower()}'
        fprint()

        fprint(f'add_library({fspec_interface_target} INTERFACE)')
        fprint()
        fprint(f'target_include_directories({fspec_interface_target} INTERFACE include)')
        fprint()

        fprint(f'target_link_libraries({fspec_interface_target} INTERFACE')

        for gf in self.generated_funcs_list:
            fprint(f'        {gf.cmakelists_lib_name}')

        fprint(f'        m')
        fprint(f'        )')

        if has_registry:
            fprint()
            fprint()
            fprint(f'add_library({fspec_registry_target} STATIC ${{{fspec_src_name}}})')
            fprint(f'target_include_directories({fspec_registry_target} PUBLIC include ../eswb/src/lib/include/public)')
            fprint(f'target_link_libraries({fspec_registry_target} PUBLIC {fspec_interface_target} m)')

        self.package.cmake_target = fspec_registry_target if has_registry else fspec_interface_target

    def generate_code(self, gen_func_registry=True):
        for f in self.generated_funcs_list:
            f.generate_code()

        self.generate_cmake_lists(self.package.root_path, gen_func_registry)
        # self.generate_fspecs_header(self.package.root_path)
        if gen_func_registry:
            self.generate_fspecs_registry_c(self.package.root_path)



def init_parser():
    import argparse

    parser = argparse.ArgumentParser('c-atom artifacts generator tool')

    parser.add_argument(
        '--code',
        help='Generate C code fir f_specs',
        action='store_true'
    )

    parser.add_argument(
        '--md',
        help='Generate markdown files for f_specs',
        action='store_true'
    )

    parser.add_argument(
        '--cmake_vars',
        help='Stdout variable for cmake integration',
        action='store_true'
    )


    parser.add_argument(
        '--bbxml',
        action='store',
        type=str,
        help='Generate building blocks description in specified xml file',
    )

    parser.add_argument('--registry_c',
                        action='store',
                        type=str,
                        help='Full path to generate C code file for centralized registry for functions from all mentioned packages'
    )

    parser.add_argument(
        '--cmake',
        help='Generate cmake file with variables',
        action='store_true'
    )

    parser.add_argument('--f_specs_dirs',
                        help='specifies list of dirs of fspecs in a format pkg_name:pkg_root_path',
                        nargs='+')

    return parser

def find_f_spec_packages(rootdir):
    rv = []
    for it in os.scandir(rootdir):
        if it.is_dir():
            rv.append(it.path)
    return rv

if __name__ == "__main__":

    parser = init_parser()
    args = parser.parse_args(sys.argv[1:])

    pkgs = []
    for fspec_specifier in args.f_specs_dirs:

        name = ''
        path = ''

        try:
            [name, path] = fspec_specifier.split(':')
        except:
            if not name or not path:
                name = os.path.basename(os.path.normpath(fspec_specifier))
                path = fspec_specifier + '/'

        if not name or not path:
            raise Exception("Name and path must be specified in format name:path ")

        pkgs.append(ctlst.load(name, path))
        # pkgs += find_f_spec_packages(fsp)

    # ctlst.load(['core'])

    for p in pkgs:
        p.resolve_types(pkgs)

    central_registry = False
    if args.registry_c and len(args.registry_c) > 0:
        central_registry = True

    for p in pkgs:
        gp = GPackage(p)
        gp.process_types()
        gp.process_constants()
        gp.process_structures()

        fp = FuncProcessor(p, pkgs)

        if args.code:
            fp.generate_code(not central_registry)

    fp = FuncProcessor(pkgs[0], pkgs, True)

    if central_registry:

        (path, fp.fspec_registry_c_filename) = os.path.split(args.registry_c)
        fp.generate_fspecs_registry_c(path)

    if (args.bbxml):
        fp.generate_bbxml(args.bbxml)

    if (args.cmake):
        fprint = new_file_write_call('./fspecs.cmake')
        for p in pkgs:
            fprint(f'add_subdirectory({os.path.relpath(p.root_path, start=os.curdir)})')

        fprint()
        fprint('set(FSPECS_LIBS')
        for p in pkgs:
            fprint(f'    {p.cmake_target}')
        fprint(')')
        if central_registry:
            fprint()
            fprint(f'set(FSPECS_REG {args.registry_c})')


    if (args.cmake_vars):
        for p in pkgs:
            print(f'SUBDIR={os.path.relpath(p.root_path, start=os.curdir)}')
            print(f'TARGET={p.cmake_target}')
