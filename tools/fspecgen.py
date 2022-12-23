#!/usr/bin/env python3
import sys

import fspeclib
import os.path
from typing import *
from common import *

override_implementation = False

src_files_dict = dict()

catom_root_path = None  # 'c-atom/eswb/src/lib/include/public'


def get_eswb_include_path():
    return f'{catom_root_path}/eswb/src/lib/include/public'


def get_function_h_include_path():
    return f'{catom_root_path}/function'


def create_generated_file(*, full_path: str, tag: str ):
    if tag != 'None':
        if not tag in src_files_dict:
            src_files_dict[tag] = []

        src_files_dict[tag].append(full_path)

    return open(full_path, 'w')


def wrap_if_required(expr, parent_expr):
    if type(expr) in [fspeclib.ThisValue, int, float]:
        return expr
    return f'{expr}'


def generate_expression(expr, map_name):
    if isinstance(expr, fspeclib.ParameterValue):
        return map_name(expr.name)
    elif type(expr) in [int, float]:
        return f'{expr}'
    elif type(expr) is fspeclib.ExpressionBinaryOperation:
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
    def __init__(self, spec: fspeclib.Type):
        self.spec = spec
        self.h_path = spec.get_common_h_filepath()
        self.rel_h_path = os.path.relpath(self.h_path)

    def generate(self):
        common_file = create_generated_file(full_path=self.h_path, tag='types')

        def fprint(*args, **kwargs):
            print(*args, **kwargs, file=common_file)

        type_name = self.spec.get_escaped_name()

        header_guard = f'fspec_{type_name}_h'.upper()

        fprint(f'#ifndef {header_guard}')
        fprint(f'#define {header_guard}')
        fprint()

        fprint(f'// TODO: Declare type alias `{type_name}`')
        fprint(f'#error "Type alias `{type_name}` is not declared."')
        fprint(f'// typedef {type_name} void;')

        fprint()
        fprint(f'#endif // {header_guard}')


class GPackage:
    def __init__(self, pkg: fspeclib.Package):
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
            header_guard = f'fspec_{constant_id}_h'.upper()

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

            header_guard = f'fspec_{structure_name}_h'.upper()

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
            fprint(f'typedef struct {structure_name}')
            fprint(f'{{')

            for member in structure.fields:
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

    def dummy_implementation(self, dummy_call_name):
        rv  = f'{{\n'
        rv += f'    // TODO: Implement {dummy_call_name} method for `{self.name}`\n'
        rv += f'    #error "Function `{dummy_call_name}` is not implemented."\n'
        rv += f'}}\n'
        return rv


class SourceFile:
    def __init__(self, filename, *, title, subdir='', root_dir=''):
        self.filename = filename
        self.title = title
        self.subdir = subdir
        self.root_dir = root_dir

    def full_path(self):
        rd = self.root_dir + os.sep if self.root_dir else ''
        return rd + self.rel_path()

    def rel_path(self):
        mydir = self.subdir + os.sep if self.subdir else ''
        return mydir + self.filename


class GeneratedFunction:

    @staticmethod
    def optional_in_struct_name():
        return f'optional_inputs_flags'

    @staticmethod
    def optional_in_struct_typename(funcname: str):
        return f'{funcname}_{GeneratedFunction.optional_in_struct_name()}_t'

    @staticmethod
    def optional_input_bitfield_name(in_name: str):
        return f'{GeneratedFunction.optional_in_struct_name()}.{in_name}'

    @staticmethod
    def changed_param_bitfield_name(p_name: str):
        return f'changed_param_{p_name}'

    def func_name_prefix(self):
        return self.spec.get_prefix()

    def __init__(self, func: fspeclib.Function, processor):
        self.spec = func
        self.processor = processor
        self.spec.generator = self
        self.files = []
        self.cmake_src_list = []
        self.function_name = func.get_escaped_name()

        self.cmakelists_src_define = f'FSPEC_{self.function_name.upper()}_SRC'
        self.cmakelists_lib_name = f'fspec_{self.function_name.lower()}-static'

        self.spec_name = f'fspec_{self.spec.get_prefix()}_spec'
        self.handler_name = f'fspec_{self.spec.get_prefix()}_handler'
        self.calls_struct_name = f'fspec_{self.spec.get_prefix()}_calls'
        self.generated_code_dir = 'g'

        sources_dir = self.spec.directory

        self.file_declaration = \
            SourceFile(f'declaration.py',
                       title=f'Declaration for {self.function_name}',
                       root_dir=sources_dir)

        # user defined (generated once)
        self.gen_file_exec_c = \
            SourceFile(f'{self.function_name}_exec.c',
                       title=f'Implementation for {self.function_name}',
                       root_dir=sources_dir)

        self.gen_file_compute_params_c = \
            SourceFile(f'{self.function_name}_compute_params.c',
                       title=f'Params computing implemtation for {self.function_name}',
                       root_dir=sources_dir) if self.spec.has_compute_parameters() else None

        # generated files
        self.gen_file_spec_c = \
            SourceFile(f'{self.function_name}_spec.c',
                       title=f'Specification of {self.function_name}',
                       subdir=self.generated_code_dir,
                       root_dir=sources_dir)

        self.gen_file_set_params_c = \
            SourceFile(f'{self.function_name}_set_params.c',
                       title=f'Set parameters code for {self.function_name}',
                       subdir=self.generated_code_dir,
                       root_dir=sources_dir) if self.spec.has_parameters() else None

        self.gen_file_interface_c = \
            SourceFile(f'{self.function_name}_interface.c',
                       title=f'Interface of {self.function_name}',
                       subdir=self.generated_code_dir,
                       root_dir=sources_dir)

        self.gen_file_h = \
            SourceFile(f'{self.function_name}.h',
                       title=f'Header of {self.function_name}',
                       subdir=self.generated_code_dir,
                       root_dir=sources_dir)

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
        pre_exec_init_func_args = []

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
        self.type_function_spec = GType(typename='struct function_spec')
        self.type_eswb_topic_descr = GType(typename='eswb_topic_descr_t')
        self.type_optional_inputs_flags = GType(typename=self.optional_in_struct_typename(func.get_prefix()))

        if func.has_inputs():
            exec_func_args.append(GArgument(type=self.type_inputs, varname='i', ptr=True, const=True))
            if func.has_optional_inputs():
                pre_exec_init_func_args.append(GArgument(type=self.type_optional_inputs_flags, varname='input_flags', ptr=True, const=True))

        if func.has_outputs():
            exec_func_args.append(GArgument(type=self.type_outputs, varname='o', ptr=True))
        if func.has_parameters():
            a = GArgument(type=self.type_params, varname='p', ptr=True, const=True)
            exec_func_args.append(a)
            pre_exec_init_func_args.append(a)
        if func.has_state():
            a = GArgument(type=self.type_state, varname='state', ptr=True)
            exec_func_args.append(a)
            pre_exec_init_func_args.append(a)
        if func.has_injection():
            a = GArgument(type=self.type_injection, varname='injection', ptr=True, const=True)
            exec_func_args.append(a)
            pre_exec_init_func_args.append(a)

        self.callable_exec = GCallableFunction(name=f'{self.func_name_prefix()}_exec',
                                               return_type='void',
                                               arguments=exec_func_args)

        self.callable_pre_exec_init = GCallableFunction(name=f'{self.func_name_prefix()}_pre_exec_init',
                                                        return_type='fspec_rv_t',
                                                        arguments=pre_exec_init_func_args)

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

        self.callable_interface_pre_exec_init \
            = GCallableFunction(name=f'{self.func_name_prefix()}_interface_pre_exec_init',
                                return_type='fspec_rv_t',
                                arguments=[interface_calls_arg])

        self.callable_interface_update = GCallableFunction(name=f'{self.func_name_prefix()}_interface_update',
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

        # Note: these calls are forming function_calls_t which is part of function_handler_t
        self.callable_call_init_inputs = GCallableFunction(name=f'{self.func_name_prefix()}_call_init_inputs',
                                                      return_type='fspec_rv_t',
                                                      arguments=[data_handle, connect_spec, mounting_td])

        self.callable_call_init_outputs = GCallableFunction(name=f'{self.func_name_prefix()}_call_init_outputs',
                                                      return_type='fspec_rv_t',
                                                      arguments=[data_handle, connect_spec, mounting_td, func_name_str])

        self.callable_call_exec = GCallableFunction(name=f'{self.func_name_prefix()}_call_exec',
                                                    return_type='void',
                                                    arguments=[GArgument(type=self.type_void, varname='dh', ptr=True)])

        self.callable_call_pre_exec_init = \
            GCallableFunction(name=f'{self.func_name_prefix()}_call_pre_exec_init',
                              return_type='fspec_rv_t',
                              arguments=[GArgument(type=self.type_void, varname='dh', ptr=True)])

        self.callable_call_set_params = GCallableFunction(name=f'{self.func_name_prefix()}_call_set_params',
                                                      return_type='fspec_rv_t',
                                                      arguments=[data_handle, params_pairs, initial_call_flag])

    @staticmethod
    def variable_type_str(value_type, *, force_f64=False):
        if force_f64:
            if isinstance(value_type, fspeclib.Structure):
                return value_type.get_c_type_name()
            else:
                # FIXME when we have types checks
                #value_type.get_c_type_name()
                return '/* TODO the only scalar type we have for now */\n    core_type_f64_t'
        else:
            return value_type.get_c_type_name()

    def decl_params(self, func, fprint):
        fprint(f'/**')
        fprint(f' * @brief Parameters of `{func.name}` function')
        fprint(f' */')
        fprint(f'typedef struct {func.get_prefix()}_params_{{')
        for parameter in func.parameters:
            value_type = parameter.value_type
            if type(value_type) is not fspeclib.VectorOf:
                fprint(f'    {self.variable_type_str(value_type)} {parameter.name};  /// {parameter.title.en}')
        fprint(f'}} {func.get_prefix()}_params_t;')
        fprint()

    def decl_optional_input_flags(self, func, fprint):

        if not func.has_optional_inputs():
            return False

        fprint(f'/**')
        fprint(f' * @brief Optional inputs connectivity flags structure for `{func.name}` function')
        fprint(f' */')
        fprint(f'typedef struct {{')

        for inp in func.inputs:
            if not inp.mandatory:
                fprint(f'    uint32_t {inp.name}:1;')

        fprint(f'}} {self.optional_in_struct_typename(func.get_prefix())};')
        fprint()

        return True

    def decl_inputs(self, func, fprint):
        has_optional_inputs = self.decl_optional_input_flags(func, fprint)

        fprint(f'/**')
        fprint(f' * @brief Inputs of `{func.name}` function')
        fprint(f' */')
        fprint(f'typedef struct {func.get_prefix()}_inputs_ {{')
        for inp in func.inputs:
            value_type = inp.value_type
            fprint(f'    {self.variable_type_str(value_type)} {inp.name};  /// {inp.title.en}')
        fprint()

        if has_optional_inputs:
            fprint(f'    {self.optional_in_struct_typename(func.get_prefix())} {self.optional_in_struct_name()};')

        fprint(f'}} {func.get_prefix()}_inputs_t;')
        fprint()

    def decl_outputs(self, func, fprint):
        fprint(f'/**')
        fprint(f' * @brief Outputs of `{func.name}` function')
        fprint(f' */')
        fprint(f'typedef struct {func.get_prefix()}_outputs_ {{')
        for output in func.outputs:
            value_type = output.value_type
            fprint(f'    {self.variable_type_str(value_type)} {output.name}; /// {output.title.en}')
            fprint()
        fprint(f'}} {func.get_prefix()}_outputs_t;')
        fprint()

    def decl_state(self, func, fprint):
        fprint(f'/**')
        fprint(f' * @brief State variables of `{func.name}` function')
        fprint(f' */')
        fprint(f'typedef struct {func.get_prefix()}_state_ {{')
        for variable in func.state:
            value_type = variable.value_type
            fprint(f'    {self.variable_type_str(value_type)} {variable.name};  /// {variable.title.en}')
        fprint(f'}} {func.get_prefix()}_state_t;')
        fprint()

    def decl_injection(self, func, fprint):
        fprint(f'/**')
        fprint(f' * @brief Injections for `{func.name}` function')
        fprint(f' */')
        fprint(f'typedef struct {func.get_prefix()}_injection_ {{')

        if func.injection.timedelta:
            doc = f'Time delta from previous iteration (measured in seconds)'
            fprint(f'    {self.processor.find_type("core.type.f64").get_c_type_name()} dt;  /// {doc}')
            fprint()

        if func.injection.timestamp:
            doc = f'Current timestamp (measured in microseconds)'
            fprint(f'    {self.processor.find_type("core.type.u64").get_c_type_name()} timestamp;  /// {doc}')
            fprint()

        fprint(f'}} {func.get_prefix()}_injection_t;')
        fprint()

    def decl_interface_structs(self, func, fprint):

        if func.has_inputs() or func.has_outputs():
            fprint(f'typedef struct {self.function_name}_eswb_descriptors_ {{')

            if func.has_inputs:
                for input in func.inputs:
                    fprint(f'    eswb_topic_descr_t in_{input.name};')

            if func.has_outputs():
                fprint(f'    eswb_topic_descr_t out_all;')

            fprint(f'}} {self.function_name}_eswb_descriptors_t;')
            fprint()

        fprint(f'typedef struct {self.function_name}_interface_ {{')

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
        fprint(f'typedef struct {func.get_prefix()}_params_flags_ {{')
        for parameter in func.parameters:
            fprint(f'    uint64_t {self.changed_param_bitfield_name(parameter.name)}:1;')
        fprint(f'}} {func.get_prefix()}_params_flags_t;')
        fprint()

    def get_types_includes(self):
        dependency_types = self.spec.get_dependency_types()
        includes = []
        if len(dependency_types) > 0:
            for dependency_type in dependency_types:
                if type(dependency_type) is not fspeclib.VectorOf:
                    includes.append(dependency_type.get_common_h_filename())
            includes.sort()

        return includes

    def decl_header(self, hdr_file_name):
        fprint = new_file_write_call(hdr_file_name)
        f_spec = self.spec

        header_guard = f'fspec_{self.function_name}_h'.upper()

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
            # fprint(f'#include "core_type_f64.h" // FIXME in generator eventually')  # FIXME when we will have type checking
            # fprint()

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

        if f_spec.has_pre_exec_init_call:
            fprint(f'{self.callable_pre_exec_init.prototype()}')

        fprint(f'{self.callable_exec.prototype()}')
        fprint()

        fprint(f'#endif // {header_guard}')

    def decl_impl_set_params(self, set_params_filename):

        f_spec = self.spec
        fprint = new_file_write_call(set_params_filename)

        pps = self.callable_set_params.arguments[-2].name
        ifs = self.callable_set_params.arguments[-1].name

        fprint(f'#include "{f_spec.get_name()}.h"')
        fprint(f'#include "conv.h"')
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
        fprint(f'    conv_rv_t crv = conv_rv_ok;', )
        fprint()

        fprint(f'    // Parse parameters', )
        fprint(f'    for (int i = 0; {pps}[i].alias != NULL; i++) {{', )
        fprint('        ', end='', )
        for parameter in f_spec.parameters:
            if parameter != f_spec.parameters[0]:
                fprint(' else ', end='', )
            fprint(f'if (strcmp({pps}[i].alias, "{parameter.name}") == 0) {{', )

            converter_func_name = ''
            not_supp = False
            if parameter.value_type.name == 'core.type.f64':
                converter_func_name = 'conv_str_double'
            elif parameter.value_type.name == 'core.type.u64':
                converter_func_name = 'conv_str_uint64'
            elif parameter.value_type.name == 'core.type.i64':
                converter_func_name = 'conv_str_int64'
            elif parameter.value_type.name == 'core.type.u32':
                converter_func_name = 'conv_str_uint32'
            elif parameter.value_type.name == 'core.type.i32':
                converter_func_name = 'conv_str_int32'
            elif parameter.value_type.name == 'core.type.u16':
                converter_func_name = 'conv_str_uint16'
            elif parameter.value_type.name == 'core.type.i16':
                converter_func_name = 'conv_str_int16'
            elif parameter.value_type.name == 'core.type.u8':
                converter_func_name = 'conv_str_uint8'
            elif parameter.value_type.name == 'core.type.i8':
                converter_func_name = 'conv_str_int8'
            elif parameter.value_type.name == 'core.type.bool':
                converter_func_name = 'conv_str_bool'
            else:
                converter_func_name = 'conv_str_not_supported'
                not_supp = True

            if not_supp:
                fprint(f'            #error not supported type ({parameter.value_type.name}) for parameter \"{parameter.name}\" parsing')
            fprint(f'            if ((crv = {converter_func_name}({pps}[i].value, &p.{parameter.name})) == conv_rv_ok) {{')
            fprint(f'                flags.{self.changed_param_bitfield_name(parameter.name)} = 1;', )
            fprint(f'            }}')
            # params.Kp = atof(param_str[i].value);
            fprint('        }', end='', )
            pass
        fprint(' else {', )
        fprint(f'            error("failed unsupported parameter \'%s\'", {pps}[i].alias);', )
        fprint(f'            violation_count ++;', )
        fprint('        }', )
        fprint()
        fprint(f'        if (crv != conv_rv_ok) {{')
        fprint(f'            error("Parameter \'%s\' format error (%s)", {pps}[i].alias, {pps}[i].value);')
        fprint(f'            violation_count++;')
        fprint(f'        }}')
        fprint('    }', )
        fprint()

        fprint(f'    if ({ifs}) {{', )
        if f_spec.has_mandatory_parameters():
            fprint('        // Check all mandatory parameters are set', )
            for parameter in f_spec.parameters:
                if not parameter.mandatory:
                    continue
                fprint(f'        if (!flags.{self.changed_param_bitfield_name(parameter.name)}) {{', )
                fprint(f'            error("failed missed mandatory parameter \'{parameter.name}\'");', )
                fprint(f'            violation_count ++;', )
                fprint(f'        }}', )

        if f_spec.has_non_mandatory_parameters():
            if f_spec.has_mandatory_parameters():
                fprint()
            fprint('        // Set default value for non mandatory parameters which are not set', )
            for parameter in f_spec.parameters:
                if parameter.mandatory or type(parameter) == fspeclib.ComputedParameter:
                    continue
                fprint(f'        if (!flags.{self.changed_param_bitfield_name(parameter.name)}) {{', )
                fprint(f'            p.{parameter.name} = {parameter.default};', )
                fprint(f'            flags.{self.changed_param_bitfield_name(parameter.name)} = 1;', )
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

        if self.spec.has_pre_exec_init_call:
            fprint(f'{self.callable_pre_exec_init.declaration()}')
            fprint(f'{self.callable_pre_exec_init.dummy_implementation(self.callable_exec.name)}')

        fprint(f'{self.callable_exec.declaration()}')
        fprint(f'{self.callable_exec.dummy_implementation(self.callable_exec.name)}')

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
            fprint()

        if self.spec.has_outputs():
            fprint(f'{self.callable_call_init_outputs.prototype()}')
            fprint()

        if self.spec.has_pre_exec_init_call:
            fprint(f'{self.callable_call_pre_exec_init.prototype()}')
            fprint()

        fprint(f'{self.callable_call_exec.prototype()}')
        fprint()

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
        pre_exec_init = self.callable_call_pre_exec_init.symbol() if self.spec.has_pre_exec_init_call else 'NULL'

        fprint(f'    .pre_exec_init = {pre_exec_init},')
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

    @staticmethod
    def scalar_topic_typename(typename: str):
        if typename == 'core.type.f64':
            return 'tt_double'
        # elif typename == 'core.type.f32':
        #     return 'tt_float'
        # elif typename == 'core.type.u32':
        #     return 'tt_uint32'
        # elif typename == 'core.type.i32':
        #     return 'tt_int32'
        # elif typename == 'core.type.u16':
        #     return 'tt_uint16'
        # elif typename == 'core.type.i16':
        #     return 'tt_uint16'
        elif typename == 'core.type.bool':
            return 'tt_int32'
        else:
            raise Exception(f'No type conversion from C-ATOM type \'{typename}\' to ESWB type')

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
                    fprint(f'        interface->i.{self.optional_input_bitfield_name(inp.name)} = 1;')
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

                additional_bracket = False
                if not input.mandatory:
                    fprint(f'    if (interface->i.{self.optional_input_bitfield_name(input.name)}) {{')
                    indent = '    '
                    additional_bracket = True
                else:
                    indent = ''

                fprint(f'{indent}    rv = {getter}(interface->eswb_descriptors.in_{input.name}, &interface->i.{input.name});')
                fprint(f'{indent}    if(rv != eswb_e_ok) {{')
                fprint(f'{indent}        /*FIXME nothing to do yet*/')
                fprint(f'{indent}    }}')
                if additional_bracket:
                    fprint(f'    }}')

                fprint()
            fprint(f'    return 0;')
            fprint(f'}}')
            fprint()

        if f_func.has_inputs():
            impl_inputs_init()
            impl_inputs_update()

        def impl_outputs_init():
            def declare_structure_elems(root_node_name: str, struct_type_name: str, outputs: List[fspeclib.Output], *, dry_run: bool):
                tree_elem_num = 0
                for output in outputs:
                    structure_type = isinstance(output.value_type, fspeclib.Structure)
                    tree_sub_root = output.name + '_s_root' if structure_type else ''
                    tree_sub_root_assignment = f'topic_proclaiming_tree_t* {tree_sub_root} = ' if structure_type else ''
                    typename = 'tt_struct' if structure_type else self.scalar_topic_typename(output.value_type.name)
                    tree_elem_num += 1
                    if not dry_run:
                        fprint(f'    {tree_sub_root_assignment}usr_topic_add_struct_child(cntx, {root_node_name}, '
                               f'{struct_type_name}, '
                               f'{output.name}, '   # for offset calc
                               f'"{output.name}", ' # name 
                               f'{typename});')     # eswb type

                    if structure_type:
                        tree_elem_num += declare_structure_elems(tree_sub_root,
                                                                 output.value_type.get_c_type_name(),
                                                                 output.value_type.fields, dry_run=dry_run)  # FIXME not inheritance based prop
                        if not dry_run:
                            fprint()

                return tree_elem_num

            tree_elems = declare_structure_elems('', '', f_func.outputs, dry_run=True)

            fprint(f'{self.callable_interface_outputs_init.declaration()}')
            fprint(f'{{')
            fprint(f'    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, {tree_elems + 1});')
            fprint(f'    {self.type_outputs.get_name()} out;')
            fprint(f'    eswb_rv_t rv;')
            # fprint(f'    topic_proclaiming_tree_t *topic;')
            fprint()
            root_node_varname = 'rt'
            fprint(f'    topic_proclaiming_tree_t *{root_node_varname} = usr_topic_set_struct(cntx, out, '
                   f'{self.callable_interface_outputs_init.arguments[-1].get_symbol()});')  # last argument
            fprint()

            declare_structure_elems(root_node_varname, self.type_outputs.get_name(), f_func.outputs, dry_run=False)

            fprint(f'    rv = eswb_proclaim_tree(mounting_td, {root_node_varname}, cntx->t_num, &interface->eswb_descriptors.out_all);')
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

        def impl_interface_pre_exec_init():
            fprint(f'{self.callable_interface_pre_exec_init.declaration()}')
            fprint(f'{{')

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
            if f_func.has_optional_inputs():
                arguments.append(f'&interface->i.{self.optional_in_struct_name()}')
            if f_func.has_parameters():
                arguments.append('&interface->p')
            if f_func.has_state():
                arguments.append('&interface->state')
            if f_func.has_injection():
                arguments.append('&interface->injection')
            fprint(f'    return {self.callable_pre_exec_init.call(arguments)}')

            fprint(f'}}')
            fprint()

        def impl_interface_exec():
            fprint(f'{self.callable_interface_update.declaration()}')
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

            fprint(f'}}')
            fprint()

        if f_func.has_pre_exec_init_call:
            impl_interface_pre_exec_init()

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

        if self.spec.has_parameters():
            gen_wrapper(self.callable_call_set_params, self.callable_set_params,
                        arg_list=['&interface->p', self.callable_call_set_params.arguments[-2].name,
                                  self.callable_call_set_params.arguments[-1].name]
                        )

        if self.spec.has_inputs():
            gen_wrapper(self.callable_call_init_inputs, self.callable_interface_inputs_init,
                        arg_list=['interface', 'conn_spec', 'mounting_td'])

        if self.spec.has_outputs():
            gen_wrapper(self.callable_call_init_outputs, self.callable_interface_outputs_init,
                        arg_list=['interface', 'conn_spec', 'mounting_td', 'func_name'])

        if self.spec.has_pre_exec_init_call:
            gen_wrapper(self.callable_call_pre_exec_init, self.callable_interface_pre_exec_init)

        ## flow_update
        gen_wrapper(self.callable_call_exec, self.callable_interface_update, do_ret=False)

        fprint(close_file=True)

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

        eswb_include_path = get_eswb_include_path()
        eswb_include_rel_path = os.path.relpath(eswb_include_path, start=self.spec.directory)

        function_h_include_path = get_function_h_include_path()
        function_h_include_path_rel = os.path.relpath(function_h_include_path, start=self.spec.directory)

        fprint(f'target_include_directories({self.cmakelists_lib_name} PRIVATE {eswb_include_rel_path})')
        fprint(f'target_include_directories({self.cmakelists_lib_name} PRIVATE {function_h_include_path_rel})')
        fprint(f'target_include_directories({self.cmakelists_lib_name} PUBLIC ')
        for t in self.spec.get_dependency_types():
            fprint(f'    {os.path.relpath(t.directory, start=self.spec.directory)}')

        # # FIXME (delete) when we will have different types check
        # dirty_fix_dir = os.path.relpath(self.spec.get_dependency_types()[-1].directory + '/../f64', start=self.spec.directory)
        # fprint(f'    {dirty_fix_dir}')

        fprint(')')

        # fprint(f'target_link_libraries({self.cmakelists_lib_name} PUBLIC eswb-if)')

    def generate_code(self, as_static_lib=False):

        gf_root = self.spec.directory

        full_gen_path = f'{gf_root}/{self.generated_code_dir}'
        if not os.path.exists(full_gen_path):
            os.mkdir(full_gen_path)

        self.decl_header(self.gen_file_h.full_path())
        self.cmake_src_list.append(self.gen_file_h.rel_path())

        gitgnore_path = f'{gf_root}/.gitignore'
        if not os.path.exists(gitgnore_path):
            fprint = new_file_write_call(gitgnore_path)
            fprint(f'{self.generated_code_dir}')

        if self.spec.has_parameters():
            self.decl_impl_set_params(self.gen_file_set_params_c.full_path())
            self.cmake_src_list.append(self.gen_file_set_params_c.rel_path())

        self.decl_spec_src(self.gen_file_spec_c.full_path())
        self.cmake_src_list.append(self.gen_file_spec_c.rel_path())

        self.decl_interface_src(self.gen_file_interface_c.full_path())
        self.cmake_src_list.append(self.gen_file_interface_c.rel_path())

        exec_src_full_path = self.gen_file_exec_c.full_path()
        if not os.path.exists(exec_src_full_path) or override_implementation:
            self.decl_impl_exec(exec_src_full_path)
        self.cmake_src_list.append(self.gen_file_exec_c.rel_path())

        if self.spec.has_compute_parameters():
            compute_param_src_full_path = self.gen_file_compute_params_c.full_path()
            if not os.path.exists(compute_param_src_full_path) or override_implementation:
                self.decl_impl_compute_params(compute_param_src_full_path)
            self.cmake_src_list.append(self.gen_file_compute_params_c.rel_path())

        cmakelists_path = f'{gf_root}/CMakeLists.txt'

        cmakefile_override_implementation = True

        if not self.spec.custom_cmakefile or not os.path.exists(cmakelists_path) or cmakefile_override_implementation:
            self.generate_cmkelists(cmakelists_path, self.spec.custom_cmakefile)

    def generate_rst_doc(self, to_dir):
        ref = self.function_name.replace('_', '-')

        doc_file_path = f'{to_dir}/{ref}.txt'

        fprint = new_file_write_call(doc_file_path)
        # fprint = print

        # reference

        fprint(f'.. atomic-{ref}')

        def header(val, char, *, section):
            fprint()
            header_underscore = char * len(val)
            if not section:
                fprint(f'{header_underscore}')
            fprint(f'{val}')
            fprint(f'{header_underscore}')
            fprint()

        header(self.spec.name, '=', section=False)

        fprint()

        # description
        fprint(self.spec.description if self.spec.description else '')
        if self.spec.description:
            fprint()

        def print_table(objects: List[object], column_keys: List[str], column_headers: List[str], column_widths: List[int]):
            fprint(f'.. list-table::')
            widths = ''
            w = int (100 / len(column_keys))
            for ck in column_widths:
                widths += f'{ck} '
            fprint(f'    :widths: {widths}')
            fprint(f'    :header-rows: {1}')
            fprint()

            def print_row(elems):
                for e in elems:
                    asterix = '*' if e is elems[0] else ' '
                    fprint(f'    {asterix} - {str(e)}')

            headers = [c if c else column_keys[column_headers.index(c)] for c in column_headers]

            print_row(headers)

            for o in objects:
                row = [o.__dict__[column_keys[i]] for i in range(0, len(column_keys))]
                print_row(row)

            fprint()
            fprint()


    # inputs table
        header('Inputs', '-', section=True)
        if self.spec.inputs:
            print_table(self.spec.inputs,
                        ['name', 'value_type',  'title',    'mandatory', 'description'],
                        ['Name', 'Type',        'Title',    'Mandatory', 'Description'],
                        [15,     15,            20,         10,           40],
                        )
        else:
            fprint('Function has no inputs')

        # outputs table
        header('Outputs', '-', section=True)
        if self.spec.outputs:
            print_table(self.spec.outputs,
                        ['name', 'value_type', 'title', 'description'],
                        ['Name', 'Type',       'Title', 'Description'],
                        [15,     15,            20,     50],
                        )
        else:
            fprint('Function has no outputs')


        # params table
        header('Parameters', '-', section=True)
        if self.spec.parameters:
            print_table(self.spec.parameters,
                        ['name', 'value_type', 'title', 'mandatory', 'default',       'description'],
                        ['Name', 'Type',       'Title', 'Mandatory', 'Default value', 'Description'],
                        [15,     15,            20,     10,          10,               30],
                        )
        else:
            fprint('Function has no parameters')

        # states table
        header('State variables', '-', section=True)
        if self.spec.state:
            print_table(self.spec.state,
                        ['name', 'value_type', 'title',  'description'],
                        ['Name', 'Type',       'Title',  'Description'],
                        [15,     15,            20,      50],
                        )
        else:
            fprint('Function has no state variables')


        # generate snippet
        header('Usage XML code snippet', '-', section=True)

        fprint(f'.. code-block:: xml')
        fprint(f'    :caption: {self.function_name} snippet for FLOW configuration file')
        fprint()
        inv_name = self.spec.name.split('.')[-1]
        fprint(f'    <f name="{inv_name}" by_spec="{self.spec.name}">')
        n = 0
        for i in self.spec.inputs:
            n += 1
            optional = '   <!-- optional -->' if not i.mandatory else ''
            fprint(f'        <in alias="{i.name}">some_block_{n}/output</in>{optional}')

        n = 0
        for p in self.spec.parameters:
            n += 1
            optional = '   <!-- optional -->' if not p.mandatory else ''
            fprint(f'        <param alias="{p.name}">0.0</param>{optional}')
        fprint(f'    </f>')

        # include artifacts
        header('Function\'s artifacts', '-', section=True)

        def print_files_tabs(file_tabs: List[SourceFile]):
            fprint('.. tabs::')
            fprint()

            for ft in file_tabs:
                if ft:
                    fprint(f'    .. tab:: {ft.title}')
                    fprint()

                    def get_lang(sf: SourceFile):
                        ext = sf.filename.split('.')[-1]
                        if ext == 'c' or ext == 'h':
                            return 'c'
                        elif ext == 'py':
                            return 'python'
                        else:
                            return ''

                    lang = get_lang(ft)
                    fprint(f'        .. code-block:: {lang}')
                    fprint(f'            :caption: {ft.filename}')

                    fprint()

                    with open(ft.full_path(), 'r', encoding='utf-8') as stream:
                            lines = stream.readlines()
                            for l in lines:
                                fprint(f'            {l}', end='')
                    fprint()

        print_files_tabs([
                self.file_declaration,
                self.gen_file_exec_c,
                self.gen_file_compute_params_c,
                self.gen_file_set_params_c,
                self.gen_file_h,
                self.gen_file_spec_c,
                self.gen_file_interface_c
            ])

        return ref




class FuncProcessor:
    def __init__(self, package: fspeclib.Package, context: List[fspeclib.Package], process_context=False):
        self.generated_funcs_list: List[GeneratedFunction] = []
        self.types_list = []
        self.types_structures_list = []

        self.spec_header_filename = 'fspecs.h'

        self.package = package
        self.context = context

        if process_context:
            for pkg in context:
                for function in pkg.functions:
                    f = GeneratedFunction(function, self)
                    self.generated_funcs_list.append(f)
                for t in pkg.types:
                    self.types_list.append(t)
                for t in pkg.structures:
                    self.types_structures_list.append(t)
        else:
            for function in package.functions:
                f = GeneratedFunction(function, self)
                self.generated_funcs_list.append(f)

        self.fspec_registry_c_filename = f'{self.package.name}_registry.c'

    def find_type(self, alias):
        return fspeclib.find_type_in_context(alias, self.context)

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

        header_guard = f'fspecs_h'.upper()

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
        fprint(f'const function_handler_t* fspec_find_handler(const char *spec_name) {{')
        fprint(f'    function_handler_t * rv;')
        fprint(f'    for (int i = 0; {array_name}[i] != NULL; i++) {{')
        fprint(f'        if (strcmp({array_name}[i]->spec->name, spec_name) == 0) {{')
        fprint(f'            return functions_spec_array[i];')
        fprint(f'        }}')
        fprint(f'    }}')
        fprint(f'    return function_lookup_declared(spec_name);')
        fprint(f'}}')

    def generate_bbxml(self, fpath):
        local_fprint = new_file_write_call(f'{fpath}')

        def idnt(i):
            return i * '    '

        def render_source_file(fprint, file: SourceFile):
            if file:
                fprint(f'{idnt(2)} <source_file filename="{file.filename}" title="{file.title}" path="{file.full_path()}"/>')

        def print_fspec(fprint, func: GeneratedFunction):
            fs = func.spec
            fprint(f'{idnt(1)}<fspec type="{fs.name}" title="{fs.title}" group="{fs.namespace}">')
            if fs.description:
                fprint(f'{idnt(2)}<description>{fs.description}</description>')

            fprint(f'{idnt(2)}<inputs>')
            for i in fs.inputs:
                fprint(f'{idnt(2)}<input alias="{i.name}" required="{i.mandatory}" title="{i.title}" type="{i.value_type.name}">')
                if i.description:
                    fprint(f'{idnt(3)}{i.description}')
                fprint(f'{idnt(2)}</input>')
            fprint(f'{idnt(2)}</inputs>')

            fprint(f'{idnt(2)}<outputs>')
            for o in fs.outputs:
                fprint(f'{idnt(3)}<output alias="{o.name}" title="{o.title}" type="{o.value_type.name}">')
                if o.description:
                    fprint(f'{idnt(3)}{o.description}')
                fprint(f'{idnt(3)}</output>')
            fprint(f'{idnt(2)}</outputs>')
            fprint(f'{idnt(2)}<params>')

            for prm in fs.parameters:
                if not prm.computable:
                    fprint(f'{idnt(3)}<param alias="{prm.name}" title="{prm.title}" type="{prm.value_type.name}">')
                    if prm.description:
                        fprint(f'{idnt(4)}{prm.description}')
                    fprint(f'{idnt(3)}</param>')
            fprint(f'{idnt(2)}</params>')

            render_source_file(fprint, func.gen_file_exec_c)
            render_source_file(fprint, func.gen_file_compute_params_c)
            render_source_file(fprint, func.gen_file_h)
            render_source_file(fprint, func.gen_file_spec_c)
            render_source_file(fprint, func.gen_file_interface_c)
            render_source_file(fprint, func.gen_file_set_params_c)

            fprint(f'{idnt(1)}</fspec>')

        def print_type(fprint, tp: Union[fspeclib.Type, fspeclib.Structure]):
            title_str = f' title="{tp.title}"' if tp.title else ''

            tag = 'type' if not isinstance(tp, fspeclib.Structure) else 'type_structure'

            fprint(f'    <{tag} name="{tp.name}"{title_str}>')
            if tp.description:
                fprint(f'        <description>{tp.description}</description>')
            if isinstance(tp, fspeclib.Structure):
                for field in tp.fields:
                    fprint(f'         <field name="{field.name}" title="{field.title}" type="{field.value_type.name}"/>')
                    if field.description:
                        fprint(f'            <description>{field.description}</description>')
            fprint(f'    </{tag}>')

        local_fprint('<fspecs>')

        for t in [*self.types_list, *self.types_structures_list]:
            print_type(local_fprint, t)

        local_fprint()

        for f in self.generated_funcs_list:
            print_fspec(local_fprint, f)

        local_fprint('</fspecs>')

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

    def generate_docs(self, dir2docs):
        doc_files_list = []

        for f in self.generated_funcs_list:
            fn = f.generate_rst_doc(dir2docs)
            doc_files_list.append(fn)

        atomics_doc_list = f'{dir2docs}/catom-atomic-catalog.txt'

        fprint = new_file_write_call(atomics_doc_list)

        fprint('.. _catom-atomic-catalog:')
        fprint()

        fprint('========================')
        fprint('Atomic functions catalog')
        fprint('========================')
        fprint()
        fprint()
        fprint('.. toctree::')
        fprint('   :titlesonly:')
        fprint()

        for f in doc_files_list:
            fprint(f'   {f}')


def init_parser():
    import argparse

    parser = argparse.ArgumentParser('c-atom artifacts generator tool')


    parser.add_argument(
        '--catom_path',
        action='store',
        default='c-atom/',
        type=str,
        help='Define path to c-atom submodule',
    )


    parser.add_argument(
        '--code',
        help='Generate C code for f_specs',
        action='store_true'
    )

    parser.add_argument(
        '--doc',
        action='store',
        type=str,
        help='Generate RST documentation for atomic functions to the designated dir',
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

    catom_root_path = args.catom_path

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

        pkgs.append(fspeclib.load(name, path))
        # pkgs += find_f_spec_packages(fsp)

    # fspeclib.load(['core'])

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

    if args.bbxml:
        fp.generate_bbxml(args.bbxml)

    if args.cmake:
        fprint = new_file_write_call('./fspecs.cmake')
        fprint('include_directories(c-atom/function)')

        fprint()

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

    if args.cmake_vars:
        for p in pkgs:
            print(f'SUBDIR={os.path.relpath(p.root_path, start=os.curdir)}')
            print(f'TARGET={p.cmake_target}')

    if args.doc:
        fp.generate_docs(args.doc)

