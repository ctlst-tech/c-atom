import os.path
from abc import ABCMeta, abstractmethod
from typing import *
import traceback
import re
import glob
import importlib
import importlib.machinery
import importlib.util


def find_type_in_context(type_alias, context):
    for pkg in context:
        t = pkg.find_type(type_alias)
        if t:
            return t

    raise Exception(f'Type with name `{type_alias}` is not declared.')


class Package:
    def __init__(self):
        self.constants = []
        self.structures: List[Structure] = []
        self.vectors: List[Vector] = []
        self.types: List[Type] = []
        self.functions: List[Function] = []
        self.root_path = ''
        self.name = ''
        self.cmake_target = None

    def find_type(self, type_name):
        for t in self.types:
            if t.name == type_name:
                return t

        for structure in self.structures:
            if structure.name == type_name:
                return structure

        for vector in self.vectors:
            if vector.name == type_name:
                return vector

        return None
        #

    def resolve_types(self, context):
        # Resolve type reference in structures
        # TODO: Check recursion

        def resolve_lambda(type_alias: str):
            return find_type_in_context(type_alias, context)

        for structure in self.structures:
            for field in structure.fields:
                field.value_type = field.value_type.resolve(None, resolve_lambda)

        for vector in self.vectors:
            try:
                vector.elem_type = vector.elem_type.resolve(None, resolve_lambda)
            except:
                pass

        # Resolve type reference in function
        for function in self.functions:
            for parameter in function.parameters:
                parameter.value_type = parameter.value_type.resolve(function, resolve_lambda)

            for inp in function.inputs:
                inp.value_type = inp.value_type.resolve(function, resolve_lambda)

            for output in function.outputs:
                output.value_type = output.value_type.resolve(function, resolve_lambda)

            for variable in function.state:
                variable.value_type = variable.value_type.resolve(function, resolve_lambda)


curr_pkg = Package()


class Expression:
    def __init__(self):
        pass

    def __add__(self, other):
        return ExpressionBinaryOperation(self, other, '+')

    def __sub__(self, other):
        return ExpressionBinaryOperation(self, other, '-')

    def __mul__(self, other):
        return ExpressionBinaryOperation(self, other, '*')

    def __truediv__(self, other):
        return ExpressionBinaryOperation(self, other, '/')

    def __lt__(self, other):
        return ExpressionBinaryOperation(self, other, '<')

    def __gt__(self, other):
        return ExpressionBinaryOperation(self, other, '>')

    def __le__(self, other):
        return ExpressionBinaryOperation(self, other, '<=')

    def __ge__(self, other):
        return ExpressionBinaryOperation(self, other, '>=')

    def __eq__(self, other):
        return ExpressionBinaryOperation(self, other, '==')

    def __ne__(self, other):
        return ExpressionBinaryOperation(self, other, '!=')

    def __and__(self, other):
        return ExpressionBinaryOperation(self, other, '&&')

    def __or__(self, other):
        return ExpressionBinaryOperation(self, other, '||')


class ExpressionValue(Expression):
    def __init__(self):
        super().__init__()


class ExpressionBinaryOperation(Expression):
    def __init__(self, lhs, rhs, operator):
        super().__init__()
        self.lhs = lhs
        self.rhs = rhs
        self.operator = operator


class ParameterValue(Expression):
    def __init__(self, name: str):
        super().__init__()
        self.name = name


class ThisValue(ParameterValue):
    def __init__(self):
        super().__init__('__this_value__')


# def it():
#     return ExpressionValue()
#
#
# def constant(name):
#     return ExpressionValue()
#
#
# def parameter(name):
#     return ExpressionValue()


class LocalizedString:
    def __init__(self, *, en, **kwargs):
        self.en = en
        for key, value in kwargs.items():
            self.__setattr__(key, value)

    def __str__(self):
        return f'{self.en}'

    @staticmethod
    def make(string):
        if type(string) == LocalizedString:
            return string
        return LocalizedString(en=string)


class ValueType(metaclass=ABCMeta):
    def __init__(self):
        pass

    @abstractmethod
    def resolve(self, function, resolving_lambda):
        return NotImplemented

    @staticmethod
    def make(value_type):
        return value_type if type(value_type) is not str else TypeReference(type_alias=value_type)


class TypeReference(ValueType):
    def __init__(self, *,
                 type_alias: str):
        super().__init__()
        self.type_alias = type_alias

    def resolve(self, function, resolving_lambda):
        return resolving_lambda(self.type_alias)
        # return pkg.resolve_type(self.type_alias)


class Parameter:
    def __init__(self, *,
                 name: str,
                 title: Union[str, LocalizedString],
                 description: Optional[str] = None,
                 value_type: Union[str, ValueType],
                 mandatory: bool = True,
                 tunable: bool = False,
                 default=None,
                 unit: Optional[str] = None,
                 constraints: Optional[List[Expression]] = None):
        self.name = name
        self.title = LocalizedString.make(title)
        self.description = description
        self.value_type = ValueType.make(value_type)
        self.tunable = tunable
        self.constraints = constraints if constraints is not None else []
        self.mandatory = False if default is not None else mandatory
        self.default = default
        self.computable = False


class ComputedParameter(Parameter):
    def __init__(self, *,
                 name: str,
                 title: Union[str, LocalizedString],
                 description: Optional[str] = None,
                 value_type: Union[str, ValueType],
                 unit: Optional[str] = None):
        super().__init__(
            name=name,
            title=title,
            description=description,
            value_type=value_type,
            mandatory=False,
            tunable=False,
            default=None,
            unit=unit
        )
        self.computable = True


class ParameterRef:
    def __init__(self, param_name: str):
        self.param_name = param_name
        self.param: Parameter = None

    def resolve(self, f):
        for p in f.parameters:
            if p.name == self.param_name:
                self.param = p
                return

        raise Exception(f'Parameter reference {self.param_name} is not resolved for function {f.name}')


class VectorTypeRefName:
    def __init__(self):
        pass

    def __get__(self, obj, objtype=None):
        return obj.vector_type.name


class VectorTypeRef(ValueType):

    name = VectorTypeRefName()

    def __init__(self, *,
                 vector_type_name: Union[str, ValueType], size: Union[int, ParameterRef]):
        super().__init__()
        self.vector_type = ValueType.make(vector_type_name)
        self.size = size  # if size is int - it is implicit size set

    def resolve(self, function, resolving_lambda):
        if not isinstance(self.vector_type, Type):
            self.vector_type = self.vector_type.resolve(function, resolving_lambda)

        if isinstance(self.size, ParameterRef):
            self.size.resolve(function)

        return self
    def get_c_type_name(self):
        self.vector_type.get_c_type_name()

class Input:
    def __init__(self, *,
                 name: str,
                 title: Union[str, LocalizedString],
                 description: Optional[Union[str, LocalizedString]] = None,
                 value_type: Union[str, ValueType],
                 unit: Optional[str] = None,
                 mandatory: bool = True):
        self.name = name
        self.title = LocalizedString.make(title)
        self.description = description
        self.value_type = ValueType.make(value_type)
        self.mandatory = mandatory


class Output:
    def __init__(self, *,
                 name: str,
                 title: Union[str, LocalizedString],
                 description: Optional[Union[str, LocalizedString]] = None,
                 value_type: Union[str, ValueType],
                 explicit_update=False,
                 unit: Optional[str] = None):
        self.name = name
        self.title = LocalizedString.make(title)
        self.description = description
        self.value_type = ValueType.make(value_type)
        self.explicit_update = explicit_update


class Variable:
    def __init__(self, *,
                 name: str,
                 title: Union[str, LocalizedString],
                 description: Optional[Union[str, LocalizedString]] = None,
                 value_type: Union[str, ValueType]):
        self.name = name
        self.title = LocalizedString.make(title)
        self.value_type = ValueType.make(value_type)
        self.description = description
        pass


declaration_filename = 'declaration.py'


class Declarable:
    def __init__(self, *,
                 name: str,
                 title: Union[str, LocalizedString],
                 description: Optional[Union[str, LocalizedString]],
                 omit_validation: bool = False):
        self.name = name
        self.title = LocalizedString.make(title)
        self.description = LocalizedString.make(description) if description else None
        self.directory = None
        self.namespace = None

        for frame in traceback.extract_stack():
            if frame.filename.endswith(declaration_filename):
                self.pkg_rel_declaration_path = os.path.relpath(frame.filename, start=curr_pkg.root_path)

                # self.directory = frame.filename[:-len(f'/{declaration_filename}')]
                (self.directory, _) = os.path.split(frame.filename)
                self.pkg_rel_directory = self.pkg_rel_declaration_path[:-len(f'/{declaration_filename}')]

                module_path = re.sub(r'/', '.', re.sub(r'(\./)', '', self.directory))

                if not module_path.endswith(self.name):
                    raise Exception('Declaration does not correspond with filesystem hierarchy naming convention')

                s = self.name.rsplit('.')
                self.namespace = f'{s[0]}.{s[1]}'

                break

        if not omit_validation:
            if self.directory is None:
                raise Exception('Declaration name could not be validated')
            else:
                self.rel_directory = os.path.relpath(self.pkg_rel_directory)

    def get_escaped_name(self):
        return self.name.replace('.', '_')

    def get_common_h_filename(self):
        return f'{self.get_escaped_name()}.h'

    def get_common_h_filepath(self):
        return f'{self.directory}/{self.get_common_h_filename()}'


class Constant(Declarable):
    def __init__(self, *,
                 name: str,
                 title: Union[str, LocalizedString],
                 description: Optional[Union[str, LocalizedString]] = None,
                 value,
                 value_type: Union[str, ValueType],
                 unit: Optional[str] = None):
        super().__init__(name=name, title=title, description=description)
        self.value = value
        self.value_type = ValueType.make(value_type)
        self.unit = unit
        curr_pkg.constants.append(self)


class Injection:
    def __init__(self, *,
                 timedelta: bool = False,
                 timestamp: bool = False):
        self.timedelta = timedelta
        self.timestamp = timestamp
        pass

    def empty(self):
        return not any([
            self.timedelta,
            self.timestamp
        ])


class Function(Declarable):
    def __init__(self, *,
                 name: str,
                 title: Union[str, LocalizedString],
                 description: Optional[Union[str, LocalizedString]] = None,
                 parameters: Optional[List[Parameter]] = None,
                 parameter_constraints: Optional[List[Expression]] = None,
                 inputs: Optional[List[Input]] = None,
                 outputs: Optional[List[Output]] = None,
                 state: Optional[List[Variable]] = None,
                 injection: Optional[Injection] = None,
                 has_pre_exec_init_call = False,
                 custom_cmakefile: bool = False,
                 dynamic: bool = False):
        super().__init__(name=name, title=title, description=description, omit_validation=dynamic)
        self.parameters = parameters if parameters is not None else []
        self.mutual_parameter_constraints = parameter_constraints if parameter_constraints else []
        self.inputs = inputs if inputs is not None else []
        self.outputs = outputs if outputs is not None else []
        self.state = state if state is not None else []
        self.injection = injection if injection is not None else Injection()
        self.custom_cmakefile = custom_cmakefile
        self.has_pre_exec_init_call = has_pre_exec_init_call
        self.generator = None

        def check_container(cnt, cnt_name: str, expected_class):
            for e in cnt:
                if not isinstance(e, expected_class):
                    raise Exception(f'Function {self.name} got \'{e.__class__.__name__}\' instead of \'{expected_class.__name__}\' in \'{cnt_name}\'')

        check_container(self.inputs, 'inputs', Input)
        check_container(self.outputs, 'outputs', Output)
        check_container(self.state, 'state', Variable)
        check_container(self.parameters, 'parameters', Parameter)

        if not dynamic:
            curr_pkg.functions.append(self)

    def get_prefix(self):
        return super().get_escaped_name()

    def get_name(self):
        return super().get_escaped_name()

    def has_inputs(self):
        return len(self.inputs) > 0

    def has_optional_inputs(self):
        for inp in self.inputs:
            if not inp.mandatory:
                return True
        return False

    @staticmethod
    def vector_var_in_container(cnt):
        rv = []
        for e in cnt:
            if isinstance(e.value_type, VectorTypeRef):
                rv.append(e)

        return rv

    def vector_inputs(self):
        return self.vector_var_in_container(self.inputs)

    def vector_outputs(self):
        return self.vector_var_in_container(self.outputs)

    def vector_state_vars(self):
        return self.vector_var_in_container(self.state)

    def non_vector_outputs(self):
        rv = []
        for o in self.outputs:
            if not isinstance(o.value_type, VectorTypeRef):
                rv.append(o)

        return rv

    def explicitly_updated_outputs(self):
        rv = []
        for o in self.outputs:
            if o.explicit_update:
                rv.append(o)

        return rv

    def has_outputs(self):
        return len(self.outputs) > 0

    def has_parameters(self):
        return len(self.parameters) > 0

    def str_parameters(self):
        return [p for p in self.parameters if p.value_type.name == 'core.type.str']

    def has_mandatory_parameters(self):
        return any(map(lambda p: p.mandatory, self.parameters)) > 0

    def has_non_mandatory_parameters(self):
        return any(map(lambda p: (not p.mandatory) and (type(p) is not ComputedParameter), self.parameters)) > 0

    def has_compute_parameters(self):
        return any(map(lambda p: type(p) == ComputedParameter, self.parameters)) > 0

    def has_tunable_parameters(self):
        return any(map(lambda p: p.tunable, self.parameters))

    def has_state(self):
        return len(self.state) > 0

    def has_injection(self):
        return not self.injection.empty()

    @staticmethod
    def find(lst: List, alias: str):
        for e in lst:
            if e.name == alias:
                return e
        return None

    def find_param(self, alias: str):
        return self.find(self.parameters, alias)

    def find_input(self, alias: str):
        return self.find(self.inputs, alias)

    def get_dependency_types(self):
        dependency_types = set()

        def collect_dependancies(dp, container):
            def collect_struct_fields(dp, s: Structure):
                d = s.get_dependency_types()
                for j in d:
                    dp.add(j)

            for e in container:
                dp.add(e.value_type)

                if isinstance(e.value_type, Structure):
                    collect_struct_fields(dp, e.value_type)
                elif isinstance(e.value_type, VectorTypeRef):
                    dp.add(e.value_type.vector_type.elem_type)
                    if isinstance(e.value_type.vector_type, Structure):
                        collect_struct_fields(dp, e.value_type.vector_type)

        collect_dependancies(dependency_types, self.parameters)
        collect_dependancies(dependency_types, self.inputs)
        collect_dependancies(dependency_types, self.outputs)
        collect_dependancies(dependency_types, self.state)

        if None in dependency_types:
            dependency_types.remove(None)

        return list(dependency_types)


class Type(Declarable):
    def __init__(self, *,
                 name: str,
                 title: Union[str, LocalizedString],
                 description: Optional[Union[str, LocalizedString]] = None):
        super().__init__(name=name, title=title, description=description)
        curr_pkg.types.append(self)

    def get_c_type_name(self):
        return super().get_escaped_name() + '_t'

    def __str__(self):
        return self.name


class Field:
    def __init__(self, *,
                 name: str,
                 title: Union[str, LocalizedString],
                 description: Optional[str] = None,
                 value_type: Union[str, ValueType]):
        self.name = name
        self.title = LocalizedString.make(title)
        self.description = description
        self.value_type = ValueType.make(value_type)


class Structure(Declarable):
    def __init__(self, *,
                 name: str,
                 title: Union[str, LocalizedString],
                 description: Optional[Union[str, LocalizedString]] = None,
                 fields: List[Field]):
        super().__init__(name=name, title=title, description=description)
        self.fields: List[Field] = fields
        curr_pkg.structures.append(self)

    def get_c_type_name(self):
        return super().get_escaped_name() + '_t'

    def get_dependency_types(self):
        dependency_types = set()

        for member in self.fields:
            dependency_types.add(member.value_type)

        if None in dependency_types:
            dependency_types.remove(None)

        return list(dependency_types)


class Vector(Declarable):
    def __init__(self, *,
                 name: str = '',
                 elem_type: Union[str, ValueType],
                 title: Union[str, LocalizedString],
                 description: Optional[Union[str, LocalizedString]] = None):
        if not name:
            etn = elem_type.replace('.', '_')
            name = f'vector_{etn}'

        super().__init__(name=f'{name}', title=title, description=description)
        self.elem_type = ValueType.make(elem_type)
        curr_pkg.vectors.append(self)

    def resolve(self, function, resolving_lambda):
        if not isinstance(self.elem_type, Type):
            self.elem_type = self.elem_type.resolve(function, resolving_lambda)

        return self

    def get_c_type_name(self):
        return super().get_escaped_name() + '_t'


def load(package_name: str, package_path: str):
    global curr_pkg
    curr_pkg.root_path = package_path
    curr_pkg.name = package_name

    for decl_file in sorted(glob.glob(f'{package_path}/**/declaration.py', recursive=True)):
        (path, file) = os.path.split(decl_file)

        loader = importlib.machinery.SourceFileLoader(file, decl_file)
        spec = importlib.util.spec_from_loader(file, loader)
        module = importlib.util.module_from_spec(spec)
        loader.exec_module(module)

    rv = curr_pkg

    curr_pkg = Package()

    return rv


def find_function(alias: str, pkg):
    for f in pkg.functions:
        if f.name == alias:
            return f

    return None

