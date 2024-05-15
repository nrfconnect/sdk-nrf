# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause


class Node:
    id: str
    kind: str = ''
    name: str = ''
    file: str = ''
    line: str = ''
    parent_ids: 'set[str] | None' = None
    children_ids: 'set[str] | None' = None
    desc: str = ''
    def __init__(self, id: str, name: str):
        self.id = id
        self.name = name
    def get_short_id(self):
        return self.kind + ':' + str(self.name)
    def add_parent(self, parent: str):
        if not self.parent_ids:
            self.parent_ids = set()
        self.parent_ids.add(parent)
    def add_child(self, child: str):
        if not self.children_ids:
            self.children_ids = set()
        self.children_ids.add(child)


class File(Node):
    kind: str = 'file'


class Group(Node):
    kind: str = 'group'
    title: str = ''


class SimpleNode(Node):
    type: str = ''


class StructField(SimpleNode):
    kind: str = 'field'
    index: int = 0


class Struct(Node):
    kind: str
    is_union: bool
    fields: 'list[StructField]'
    def __init__(self, id: str, name: str, is_union: bool):
        super().__init__(id, name)
        self.is_union = is_union
        self.kind = 'union' if is_union else 'struct'
        self.fields = []


class Param:
    index: int
    name: str
    type: str
    desc: str


class FunctionLike(Node):
    params: 'list[Param]'
    def __init__(self, id: str, name: str):
        super().__init__(id, name)
        self.params = []
    def add_param(self):
        param = Param()
        param.index = len(self.params)
        self.params.append(param)
        return param


class Function(FunctionLike):
    kind: str = 'func'
    return_type: str = 'void'


class Define(FunctionLike):
    kind: str = 'def'
    value: str = ''


class EnumValue(Node):
    kind: str = 'enum_value'
    value: str


class Enum(Node):
    kind: str = 'enum'


class Typedef(SimpleNode):
    kind: str = 'typedef'


class Variable(SimpleNode):
    kind: str = 'var'
