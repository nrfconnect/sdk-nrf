# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import sys
import pickle
from pathlib import Path
from dts_tools import devicetree_sources, warning

if devicetree_sources:
    sys.path.insert(0, devicetree_sources)

from devicetree import edtlib


class ParseResult:
    bindings: 'list[Binding]'
    binding_by_name: 'dict[str, Binding]'
    def __init__(self):
        self.bindings = []
        self.binding_by_name = {}

class Property:
    name: str
    type: str
    description: str
    enum: 'set[str]'
    const: 'str | None'
    default: 'str | None'
    deprecated: bool
    required: bool
    specifier_space: str

    def __init__(self, prop: edtlib.PropertySpec):
        self.name = prop.name
        self.type = prop.type or ''
        self.description = prop.description or ''
        self.enum = { str(x) for x in (prop.enum or []) }
        self.const = str(prop.const) if prop.const else None
        self.default = str(prop.default) if prop.default else None
        self.deprecated = prop.deprecated or False
        self.required = prop.required or False
        self.specifier_space = str(prop.specifier_space or '')

class Binding:
    path: str
    name: str
    description: str
    cells: str
    buses: str
    properties: 'dict[str, Property]'

    def __init__(self, binding: edtlib.Binding, file: Path):
        self.path = str(file)
        self.name = binding.compatible or self.path
        if binding.on_bus is not None:
            self.name += '@' + binding.on_bus
        self.description = binding.description or ''
        cells_array = [
            f'{name}={";".join(value)}' for name, value in (binding.specifier2cells or {}).items()
        ]
        cells_array.sort()
        self.cells = '&'.join(cells_array)
        busses_array = list(binding.buses or [])
        busses_array.sort()
        self.buses = ';'.join(busses_array)
        self.properties = {}
        for key, value in (binding.prop2specs or {}).items():
            prop = Property(value)
            self.properties[key] = prop


def get_binding_files(bindings_dirs: 'list[Path]') -> 'list[Path]':
    binding_files = []
    for bindings_dir in bindings_dirs:
        if not bindings_dir.is_dir():
            raise FileNotFoundError(f'Bindings directory "{bindings_dir}" not found.')
        for file in bindings_dir.glob('**/*.yaml'):
            binding_files.append(file)
        for file in bindings_dir.glob('**/*.yml'):
            binding_files.append(file)
    return binding_files


def parse_bindings(dirs_or_pickle: 'list[Path]|Path') -> ParseResult:
    result = ParseResult()
    if isinstance(dirs_or_pickle, list):
        yaml_files = get_binding_files(dirs_or_pickle)
        fname2path: 'dict[str, str]' = {
            path.name: str(path) for path in yaml_files
        }
        for binding_file in yaml_files:
            try:
                binding = Binding(edtlib.Binding(str(binding_file), fname2path, None, False, False), binding_file)
                if binding.name in result.binding_by_name:
                    warning(f'Repeating binding {binding.name}: {binding.path}  {result.binding_by_name[binding.name].path}')
                result.bindings.append(binding)
                result.binding_by_name[binding.name] = binding
            except edtlib.EDTError as err:
                warning(err)
    else:
        with open(dirs_or_pickle, 'rb') as fd:
            result = pickle.load(fd)
    return result


def save_bindings(parse_result: ParseResult, file: Path):
    with open(file, 'wb') as fd:
        pickle.dump(parse_result, fd)
