# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import pickle
import re
import doxmlparser.index as dox_index
import doxmlparser.compound as dox_compound
from random import shuffle
from pathlib import Path
from json import JSONEncoder
from nodes import Define, Enum, EnumValue, File, Function, FunctionLike, Group, Node, SimpleNode, Struct, StructField, Typedef, Variable
from tools import concurrent_pool_iter, warning

HEADER_FILE_EXTENSION = '.h'

def parse_description(*args):
    return '' # Convert descriptions to string
    # <briefdescription>
    # <detaileddescription>
    # <inbodydescription>

def parse_location_description(node: Node, compound: 'dox_compound.compounddefType | dox_compound.memberdefType'):
    loc = compound.location
    if not loc:
        node.file = ''
        node.line = None
    elif hasattr(loc, 'bodyfile') and loc.bodyfile and loc.bodyfile.endswith(HEADER_FILE_EXTENSION):
        node.file = loc.bodyfile
        node.line = loc.bodystart if hasattr(loc, 'bodystart') else None
    elif hasattr(loc, 'file') and loc.file and loc.file.endswith(HEADER_FILE_EXTENSION):
        node.file = loc.file
        node.line = loc.line if hasattr(loc, 'line') else None
    elif hasattr(loc, 'declfile') and loc.declfile and loc.declfile.endswith(HEADER_FILE_EXTENSION):
        node.file = loc.declfile
        node.line = loc.declline if hasattr(loc, 'declline') else None
    elif hasattr(loc, 'bodyfile') and loc.bodyfile:
        node.file = loc.bodyfile
        node.line = loc.bodystart if hasattr(loc, 'bodystart') else None
    elif hasattr(loc, 'file') and loc.file:
        node.file = loc.file
        node.line = loc.line if hasattr(loc, 'line') else None
    elif hasattr(loc, 'declfile') and loc.declfile:
        node.file = loc.declfile
        node.line = loc.declline if hasattr(loc, 'declline') else None
    else:
        node.file = ''
        node.line = None
    node.desc = parse_description(compound)


def parse_linked_text(type: 'dox_compound.linkedTextType | None') -> str:
    if not type:
        return 'void'
    result = ''
    for part in type.content_:
        part: dox_compound.MixedContainer
        if part.category == dox_compound.MixedContainer.CategoryText:
            result += part.value
        elif (part.category == dox_compound.MixedContainer.CategoryComplex) and (part.name == 'ref'):
            value: dox_compound.refTextType = part.value
            result += value.valueOf_
    return result.strip()


def parse_function_like(node: FunctionLike, memberdef: dox_compound.memberdefType):
    parse_location_description(node, memberdef)
    for dox_param in memberdef.param:
        dox_param: dox_compound.paramType
        param = node.add_param()
        param.desc = parse_description(dox_param)
        param.name = dox_param.declname or dox_param.defname
        param.type = parse_linked_text(dox_param.get_type())

def parse_function(memberdef: dox_compound.memberdefType) -> Function:
    func = Function(memberdef.id, memberdef.name)
    parse_function_like(func, memberdef)
    func.return_type = parse_linked_text(memberdef.get_type())
    return func

def parse_define(memberdef: dox_compound.memberdefType) -> Define:
    define = Define(memberdef.id, memberdef.name)
    parse_function_like(define, memberdef)
    define.value = parse_linked_text(memberdef.initializer)
    return define

def parse_enum(memberdef: dox_compound.memberdefType, name_override: str=None) -> 'list[Enum | EnumValue]':
    result: 'list[Enum | EnumValue]' = []
    enum = Enum(memberdef.id, name_override or memberdef.name)
    parse_location_description(enum, memberdef)
    result.append(enum)
    last_value = ''
    increment = 0
    for dox_value in memberdef.enumvalue:
        dox_value: dox_compound.enumvalueType
        enum_value = EnumValue(dox_value.id, dox_value.name)
        enum_value.file = enum.file
        enum_value.line = enum.line
        enum_value.desc = parse_description(dox_value)
        enum_value.value = parse_linked_text(dox_value.initializer)
        while enum_value.value.startswith('='):
            enum_value.value = enum_value.value[1:].strip()
        if enum_value.value and (enum_value.value != 'void'):
            last_value = enum_value.value
            increment = 1
        else:
            enum_value.value = f'{last_value} + {increment}' if last_value else str(increment)
            increment += 1
        result.append(enum_value)
    return result

def parse_simple_node(node: SimpleNode, memberdef: dox_compound.memberdefType) -> SimpleNode:
    parse_location_description(node, memberdef)
    node.type = parse_linked_text(memberdef.get_type()) + (memberdef.argsstring or '')
    return node

def parse_memberdef(memberdef: dox_compound.memberdefType) -> 'list[Node]':
    result: 'list[Node]' = []
    if memberdef.kind == dox_compound.DoxMemberKind.FUNCTION:
        result.append(parse_function(memberdef))
    elif memberdef.kind == dox_compound.DoxMemberKind.DEFINE:
        result.append(parse_define(memberdef))
    elif memberdef.kind == dox_compound.DoxMemberKind.ENUM:
        result.extend(parse_enum(memberdef))
    elif memberdef.kind == dox_compound.DoxMemberKind.TYPEDEF:
        result.append(parse_simple_node(Typedef(memberdef.id, memberdef.name), memberdef))
    elif memberdef.kind == dox_compound.DoxMemberKind.VARIABLE:
        result.append(parse_simple_node(Variable(memberdef.id, memberdef.name), memberdef))
    else:
        warning(f'Unknown member kind "{memberdef.kind}".')
    return result


def parse_file_or_group(node: 'File | Group', compound: dox_compound.compounddefType) -> 'list[Node]':
    result: 'list[Node]' = [node]
    parse_location_description(node, compound)
    for inner_ref in (compound.innerclass or []) + (compound.innergroup or []):
        inner_ref: dox_compound.refType
        node.add_child(inner_ref.refid)
    for sectiondef in compound.sectiondef or []:
        sectiondef: dox_compound.sectiondefType
        for member in sectiondef.member:
            member: dox_compound.MemberType
            node.add_child(member.refid)
        for memberdef in sectiondef.memberdef or []:
            children = parse_memberdef(memberdef)
            for child in children:
                child: Node
                node.add_child(child.id)
            result.extend(children)
    return result


def parse_file(compound: dox_compound.compounddefType) -> 'list[Node]':
    file = File(compound.id, compound.compoundname)
    return parse_file_or_group(file, compound)


def parse_group(compound: dox_compound.compounddefType) -> 'list[Node]':
    group = Group(compound.id, compound.compoundname)
    group.title = compound.title
    return parse_file_or_group(group, compound)


def parse_field_with_macro(memberdef: dox_compound.memberdefType) -> StructField:
    field = StructField(memberdef.id, memberdef.name)
    parse_location_description(field, memberdef)
    argsstring: str = (memberdef.argsstring or '')
    regex = r'^\s*\(\s*([a-z_0-9]+)(?:\(.*?\)|.)*?\)(?:\s*([A-Z_0-9]+)\s*$)?'
    matches = re.search(regex, argsstring, re.IGNORECASE | re.DOTALL)
    field.type = parse_linked_text(memberdef.get_type())
    if matches:
        if len(field.type) > 0:
            field.type += ' '
        field.type += field.name
        if matches.group(2):
            field.type += (argsstring[:matches.start(2)].strip() + argsstring[matches.end(2):].strip()).strip()
            field.name = matches.group(2)
        else:
            field.type += (argsstring[:matches.start(1)].strip() + argsstring[matches.end(1):].strip()).strip()
            field.name = matches.group(1)
    else:
        field.type = parse_linked_text(memberdef.get_type()) + argsstring
    return field

def parse_struct(compound: dox_compound.compounddefType, is_union: bool) -> 'list[Node]':
    result: 'list[Node]' = []
    struct = Struct(compound.id, compound.compoundname, is_union)
    parse_location_description(struct, compound)
    for sectiondef in compound.sectiondef or []:
        sectiondef: dox_compound.sectiondefType
        for memberdef in sectiondef.memberdef or []:
            memberdef: dox_compound.memberdefType
            if memberdef.kind == dox_compound.DoxMemberKind.VARIABLE:
                field: StructField = parse_simple_node(StructField(memberdef.id, memberdef.name), memberdef)
                field.index = len(struct.fields)
                struct.fields.append(field)
            elif memberdef.kind == dox_compound.DoxMemberKind.FUNCTION:
                field = parse_field_with_macro(memberdef)
                field.index = len(struct.fields)
                struct.fields.append(field)
            elif memberdef.kind == dox_compound.DoxMemberKind.ENUM:
                full_name = memberdef.qualifiedname
                if not memberdef.name:
                    full_name += '::' + memberdef.id
                enum = parse_enum(memberdef, full_name)
                result.extend(enum)
            else:
                warning(f'Unknown structure member kind "{memberdef.kind}", name {memberdef.name} in {struct.name}, {struct.file}:{struct.line}')
    result.append(struct)
    return result


def process_compound(file_name: str) -> 'list[Node]':
    result: list[Node] = []
    for compound in dox_compound.parse(file_name, True, True).get_compounddef():
        compound: dox_compound.compounddefType
        if compound.kind == dox_index.CompoundKind.FILE:
            result.extend(parse_file(compound))
        elif compound.kind == dox_index.CompoundKind.GROUP:
            result.extend(parse_group(compound))
        elif compound.kind in (dox_index.CompoundKind.STRUCT,
                               dox_index.CompoundKind.CLASS,
                               dox_index.CompoundKind.UNION):
            result.extend(parse_struct(compound, (compound.kind == dox_index.CompoundKind.UNION)))
        else:
            warning(f'Unexpected doxygen compound kind: "{compound.kind}"')
    return result


class ParseResult:
    nodes: 'list[Node]'
    nodes_by_id: 'dict[str, Node]'
    nodes_by_short_id: 'dict[str, Node | dict[str, Node]]'
    def __init__(self):
        self.nodes = []
        self.nodes_by_id = {}
        self.nodes_by_short_id = {}


def first_node(a: Node, b: Node):
    properties = set(filter(lambda x: not x.startswith('_'), dir(a)))
    properties = list(properties.union(set(filter(lambda x: not x.startswith('_'), dir(b)))))
    properties.sort()
    for name in properties:
        a_value = getattr(a, name) if hasattr(a, name) else None
        b_value = getattr(b, name) if hasattr(b, name) else None
        if callable(a_value) or callable(b_value) or isinstance(a_value, (set, list)) or isinstance(b_value, (set, list)):
            continue
        if (a_value is None) and (b_value is None): continue
        if (a_value is None) and (b_value is not None): return False
        if (a_value is not None) and (b_value is None): return True
        a_value = str(a_value)
        b_value = str(b_value)
        if a_value == b_value:
            continue
        return a_value > b_value
    return True


def prepare_result(nodes: 'list[Node]') -> ParseResult:
    result = ParseResult()
    # Add node to nodes_by_id dictionary
    for node in nodes:
        if node.id in result.nodes_by_id:
            other = result.nodes_by_id[node.id]
            if node.get_short_id() != other.get_short_id():
                warning(f'Doxygen identifier repeated for multiple nodes: {node.id}')
            # If overriding always select the same node to ensure deterministic behavior
            result.nodes_by_id[node.id] = node if first_node(node, other) else other
        else:
            result.nodes_by_id[node.id] = node
    # Use only accessible nodes
    result.nodes = list(result.nodes_by_id.values())
    # Add node to nodes_by_short_id dictionary
    for node in result.nodes:
        short_id = node.get_short_id()
        if short_id == "def:PTHREAD_PROCESS_PRIVATE":
            short_id = short_id
        if short_id in result.nodes_by_short_id:
            # Create or update group with the same short id
            other = result.nodes_by_short_id[short_id]
            if isinstance(other, dict):
                group = other
            else:
                group = {}
                group[other.file] = other
            # Generate a key for this node
            if tuple(group.keys())[0].count('>') > 0:
                # If group contains keys with doxygen id, use file path and id as key
                key = node.file + '>' + node.id + '>' + str(node.line)
            else:
                # If group does not contain keys with doxygen id, use file path only
                key = node.file
                # In case of duplicate, convert keys to keys with doxygen id
                if key in group:
                    key += '>' + node.id + '>' + str(node.line)
                    new_group = {}
                    for group_key, group_node in group.items():
                        new_group[group_key + '>' + group_node.id + '>' + str(group_node.line)] = group_node
                    group = new_group
            # Set node and group
            group[key] = node
            result.nodes_by_short_id[short_id] = group
        else:
            result.nodes_by_short_id[short_id] = node
    # Fix parent-child relations: delete nonexisting links and create both ways link
    for node in result.nodes:
        if node.parent_ids:
            new_set = set()
            for parent_id in node.parent_ids:
                if parent_id in result.nodes_by_id:
                    new_set.add(parent_id)
                    result.nodes_by_id[parent_id].add_child(node.id)
            node.parent_ids = new_set
        if node.children_ids:
            new_set = set()
            for child_id in node.children_ids:
                if child_id in result.nodes_by_id:
                    new_set.add(child_id)
                    result.nodes_by_id[child_id].add_parent(node.id)
            node.children_ids = new_set
    return result


def save_doxygen(parse_result: ParseResult, file: Path):
    with open(file, 'wb') as fd:
        pickle.dump(parse_result.nodes, fd)


def dump_doxygen_json(parse_result: ParseResult, file: Path):
    def default_nodes(o):
        if isinstance(o, set):
            return list(o)
        else:
            d = {'__id__': id(o)}
            for name in tuple(dir(o)):
                if not name.startswith('_'):
                    value = getattr(o, name)
                    if not callable(value):
                        d[name] = value
            return d
    def default_refs(o):
        return f'__refid__{id(o)}'
    nodes_json = JSONEncoder(sort_keys=False, indent=4, default=default_nodes).encode(parse_result.nodes)
    by_id_json = JSONEncoder(sort_keys=False, indent=4, default=default_refs).encode(parse_result.nodes_by_id)
    by_short_id_json = JSONEncoder(sort_keys=False, indent=4, default=default_refs).encode(parse_result.nodes_by_short_id)
    with open(file, 'w') as fd:
        fd.write('{\n"nodes": ' + nodes_json + ',\n"by_id": ' + by_id_json + ',\n"by_short_id": ' + by_short_id_json + '\n}\n')


def parse_doxygen(dir_or_file: Path) -> ParseResult:
    nodes: 'list[Node]' = []
    if dir_or_file.is_dir():
        index = dox_index.parse(dir_or_file / 'index.xml', True, True)
        files: 'list[str]' = []
        for compound in index.get_compound():
            if compound.kind in (dox_index.CompoundKind.FILE,
                                dox_index.CompoundKind.GROUP,
                                dox_index.CompoundKind.STRUCT,
                                dox_index.CompoundKind.CLASS,
                                dox_index.CompoundKind.UNION):
                files.append(dir_or_file / (compound.refid + '.xml'))
            elif compound.kind in (dox_index.CompoundKind.PAGE,
                                dox_index.CompoundKind.DIR,
                                dox_index.CompoundKind.CATEGORY,
                                dox_index.CompoundKind.CONCEPT,
                                dox_index.CompoundKind.EXAMPLE):
                pass
            else:
                warning(f'Unknown doxygen compound kind: "{compound.kind}"')
        shuffle(files)
        #ids = ids[0:100]
        for node, _, _ in concurrent_pool_iter(process_compound, files, True, 20):
            nodes.extend(node)
    else:
        with open(dir_or_file, 'rb') as fd:
            nodes = pickle.load(fd)
    return prepare_result(nodes)
