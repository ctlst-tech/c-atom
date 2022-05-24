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
        self.structures = []
        self.types = []
        self.functions = []
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

        return None
        #

    def resolve_types(self, context):
        # Resolve type reference in structures
        # TODO: Check recursion

        def resolve_lambda(type_alias: str):
            return find_type_in_context(type_alias, context)

        for structure in self.structures:
            for member in structure.members:
                member.value_type = member.value_type.resolve(resolve_lambda)

        # Resolve type reference in function
        for function in self.functions:
            for parameter in function.parameters:
                parameter.value_type = parameter.value_type.resolve(resolve_lambda)

            for input in function.inputs:
                input.value_type = input.value_type.resolve(resolve_lambda)

            for output in function.outputs:
                output.value_type = output.value_type.resolve(resolve_lambda)

            for variable in function.state:
                variable.value_type = variable.value_type.resolve(resolve_lambda)


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
    def resolve(self, resolving_lambda):
        return NotImplemented

    @staticmethod
    def make(value_type):
        return value_type if type(value_type) is not str else TypeReference(type_alias=value_type)


class TypeReference(ValueType):
    def __init__(self, *,
                 type_alias: str):
        super().__init__()
        self.type_alias = type_alias

    def resolve(self, resolving_lambda):
        return resolving_lambda(self.type_alias)
        # return pkg.resolve_type(self.type_alias)

class VectorOf(ValueType):
    def __init__(self, *,
                 subtype: Union[str, ValueType]):
        super().__init__()
        self.subtype = ValueType.make(subtype)

    def resolve(self, resolving_lambda):
        self.subtype = self.subtype.resolve(resolving_lambda)
        return self


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
                 unit: Optional[str] = None):
        self.name = name
        self.title = LocalizedString.make(title)
        self.description = description
        self.value_type = ValueType.make(value_type)


class Variable:
    def __init__(self, *,
                 name: str,
                 title: Union[str, LocalizedString],
                 description: Optional[Union[str, LocalizedString]] = None,
                 value_type: Union[str, ValueType]):
        self.name = name
        self.title = LocalizedString.make(title)
        self.value_type = ValueType.make(value_type)
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
                (self.directory, dummy) = os.path.split(frame.filename)
                self.pkg_rel_directory = self.pkg_rel_declaration_path[:-len(f'/{declaration_filename}')]

                module_path = re.sub(r'/', '.', re.sub(r'(\./)', '', self.directory))

                if not module_path.endswith(self.name):
                    raise Exception('Declaration name is not correspond filesystem hierarchy naming convention')

                s = self.name.rsplit('.')
                self.namespace = f'{s[0]}.{s[1]}'

                break

        if not omit_validation:
            if self.directory is None:
                raise Exception('Declaration name could not be validated')
            else:
                self.rel_directory = os.path.relpath(self.pkg_rel_directory)


    def get_escaped_name(self):
        return re.sub(r'(\.)', '_', self.name)

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

        self.generator = None

        if not dynamic:
            curr_pkg.functions.append(self)



    def get_prefix(self):
        return super().get_escaped_name()

    def get_name(self):
        return super().get_escaped_name()

    def has_inputs(self):
        return len(self.inputs) > 0

    def has_outputs(self):
        return len(self.outputs) > 0

    def has_parameters(self):
        return len(self.parameters) > 0

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

        for parameter in self.parameters:
            dependency_types.add(parameter.value_type)

        for input in self.inputs:
            dependency_types.add(input.value_type)

        for output in self.outputs:
            dependency_types.add(output.value_type)

        # for variable in self.state:
        #     dependency_types.add(resolve_type(variable.name))

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


class Field:
    def __init__(self, *,
                 name: str,
                 title: Union[str, LocalizedString],
                 description: Optional[str] = None,
                 value_type: Union[str, ValueType]):
        self.name = name
        self.title = LocalizedString.make(title)
        self.value_type = ValueType.make(value_type)


class Structure(Declarable):
    def __init__(self, *,
                 name: str,
                 title: Union[str, LocalizedString],
                 description: Optional[Union[str, LocalizedString]] = None,
                 members: List[Field]):
        super().__init__(name=name, title=title, description=description)
        self.members = members
        curr_pkg.structures.append(self)

    def get_c_type_name(self):
        return super().get_escaped_name() + '_t'

    def get_dependency_types(self):
        dependency_types = set()

        for member in self.members:
            dependency_types.add(member.value_type)

        if None in dependency_types:
            dependency_types.remove(None)

        return list(dependency_types)


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

