# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from typing import Any
from nodes import Enum, EnumValue, File, Function, FunctionLike, Group, Node, Param, Struct, StructField, Typedef, Variable, Define
from dox_parser import ParseResult


ADDED = 'added'
DELETED = 'deleted'
MODIFIED = 'modified'


class GroupChanges:
    name: str
    title: str
    files: 'list[FileChanges]'
    changes: 'list[AnyChange]'
    def __init__(self, name: str, title: str) -> None:
        self.name = name
        self.title = title
        self.changes = []
        self.files = []


class FileChanges:
    name: str
    groups: 'list[GroupChanges]'
    changes: 'list[AnyChange]'
    def __init__(self, name: str) -> None:
        self.name = name
        self.changes = []
        self.groups = []


class AnyChange:
    kind: str
    action: str
    def __init__(self, action: str, new: Any, old: Any):
        self.action = action
        self.new = new
        self.old = old


class NodeChange(AnyChange):
    file: bool = False
    desc: bool = False


class TypedefChange(NodeChange):
    kind = 'typedef'
    type: bool = False
    new: Typedef
    old: Typedef


class VariableChange(NodeChange):
    kind = 'var'
    type: bool = False
    new: Variable
    old: Variable


class EnumValueChange(NodeChange):
    kind = 'enum_value'
    value: bool = False
    new: EnumValue
    old: EnumValue


class EnumChange(NodeChange):
    kind = 'enum'
    new: Variable
    old: Variable


class StructFieldChange(AnyChange):
    kind = 'field'
    index: bool = False
    type: bool = False
    desc: bool = False
    new: StructField
    old: StructField


class StructChange(NodeChange):
    kind = 'struct'
    fields: 'list[StructFieldChange]'
    new: Struct
    old: Struct
    def __init__(self, action: str, new: Any, old: Any):
        super().__init__(action, new, old)
        self.fields = []


class ParamChange(AnyChange):
    kind = 'param'
    index: bool = False
    type: bool = False
    desc: bool = False
    new: Param
    old: Param


class FunctionLikeChange(NodeChange):
    params: 'list[ParamChange]'
    def __init__(self, action: str, new: Any, old: Any):
        super().__init__(action, new, old)
        self.params = []


class FunctionChange(FunctionLikeChange):
    kind: str = 'func'
    return_type: bool = False
    new: Function
    old: Function


class DefineChange(FunctionLikeChange):
    kind: str = 'def'
    value: bool = False
    new: Define
    old: Define


def match_items(new: 'list[EnumValue | Param | StructField]', old: 'list[EnumValue | Param | StructField]') -> 'tuple[list[EnumValue | Param | StructField], list[tuple[EnumValue | Param | StructField, EnumValue | Param | StructField]], list[EnumValue | Param | StructField]]':
    def by_name(items: 'list[EnumValue | Param | StructField]'):
        result = {}
        for item in items:
            if item.name not in result:
                result[item.name] = item
        return result

    new_by_name = by_name(new)
    old_by_name = by_name(old)

    deleted = set(old_by_name.values())
    matched = []
    added = []

    for name, new_value in new_by_name.items():
        if name in old_by_name:
            matched.append((new_value, old_by_name[name]))
            if old_by_name[name] in deleted:
                deleted.remove(old_by_name[name])
        else:
            added.append(new_value)

    deleted = list(deleted)

    return deleted, matched, added


def get_add_delete_change(node: Node, action: str) -> 'list[AnyChange]':
    if isinstance(node, Typedef):
        return [TypedefChange(action, node, node)]
    elif isinstance(node, Variable):
        return [VariableChange(action, node, node)]
    elif isinstance(node, EnumValue):
        return [EnumValueChange(action, node, node)]
    elif isinstance(node, Enum):
        if node.name:
            return [EnumChange(action, node, node)]
        else:
            return []
    elif isinstance(node, Struct):
        return [StructChange(action, node, node)]
    elif isinstance(node, Function):
        return [FunctionChange(action, node, node)]
    elif isinstance(node, Define):
        return [DefineChange(action, node, node)]
    else:
        return []


def get_modification_changes(new: Node, old: Node) -> 'list[AnyChange]':
    result:'list[AnyChange]' = []

    if (new.kind != old.kind) or (type(new) is type(old)) or isinstance(new, (File, Group)):
        return []

    node_change = None
    updated = False

    if isinstance(new, Typedef):
        new: Typedef
        old: Typedef
        node_change = TypedefChange(MODIFIED, new, old)
        node_change.type = new.type != old.type
        updated = node_change.type

    elif isinstance(new, Variable):
        new: Variable
        old: Variable
        node_change = VariableChange(MODIFIED, new, old)
        node_change.type = new.type != old.type
        updated = node_change.type

    elif isinstance(new, EnumValue):
        new: EnumValue
        old: EnumValue
        node_change = EnumValueChange(MODIFIED, new, old)
        node_change.value = new.value != old.value
        updated = node_change.value

    elif isinstance(new, Enum):
        node_change = EnumChange(MODIFIED, new, old)

    elif isinstance(new, Struct):
        new: Struct
        old: Struct
        node_change = StructChange(MODIFIED, new, old)
        deleted, matched, added = match_items(new.fields, old.fields)
        for field in deleted:
            node_change.fields.append(StructFieldChange(DELETED, field, field))
        for field in added:
            node_change.fields.append(StructFieldChange(ADDED, field, field))
        for new_field, old_field in matched:
            field_change = StructFieldChange(MODIFIED, new_field, old_field)
            field_change.index = new_field.index != old_field.index
            field_change.type = new_field.type != old_field.type
            field_change.desc = new_field.desc != old_field.desc
            if field_change.index or field_change.type or field_change.desc:
                node_change.fields.append(field_change)
        updated = len(node_change.fields) != 0

    elif isinstance(new, FunctionLike):
        new: FunctionLike
        old: FunctionLike
        if isinstance(new, Function):
            node_change = FunctionChange(MODIFIED, new, old)
            node_change.return_type = new.return_type != old.return_type
            updated = node_change.return_type
        else:
            node_change = DefineChange(MODIFIED, new, old)
            node_change.value = new.value != old.value
            updated = node_change.value
        deleted, matched, added = match_items(new.params, old.params)
        for param in deleted:
            node_change.params.append(ParamChange(DELETED, param, param))
        for param in added:
            node_change.params.append(ParamChange(ADDED, param, param))
        for new_param, old_param in matched:
            param_change = ParamChange(MODIFIED, new_param, old_param)
            param_change.index = new_param.index != old_param.index
            param_change.type = new_param.type != old_param.type
            param_change.desc = new_param.desc != old_param.desc
            if param_change.index or param_change.type or param_change.desc:
                node_change.params.append(param_change)
        updated = updated or (len(node_change.params) != 0)
    else:
        raise ValueError(str(new))

    node_change.file = new.file != old.file
    node_change.desc = new.desc != old.desc

    if updated or node_change.file or node_change.desc:
        result.append(node_change)

    return result


def convert_to_long_key(group: 'dict[None]') -> dict[None]:
    result = {}
    for group_key, group_node in group.items():
        result[group_key + '>' + group_node.id + '>' + str(group_node.line)] = group_node
    return result


def match_groups(matched: 'list[tuple[Node, Node]]', added: 'list[Node]', old_matched: 'set[Node]', new_group: 'dict[str, Node]', old_group: 'dict[str, Node]'):
    new_is_long_key = tuple(new_group.keys())[0].count('>') > 0
    old_is_long_key = tuple(old_group.keys())[0].count('>') > 0
    if new_is_long_key and not old_is_long_key:
        old_group = convert_to_long_key(old_group)
    elif old_is_long_key and not new_is_long_key:
        new_group = convert_to_long_key(new_group)

    for key, new_node in new_group.items():
        if key in old_group:
            old_node = old_group[key]
            matched.append((new_node, old_node))
            old_matched.add(old_node)
        else:
            added.append(new_node)


def compare_nodes(new: ParseResult, old: ParseResult) -> 'list[AnyChange]':
    deleted: 'list[Node]' = []
    matched: 'list[tuple[Node, Node]]' = []
    added: 'list[Node]' = []
    old_matched: 'set[Node]' = set()

    for short_id, new_node in new.nodes_by_short_id.items():
        if short_id in old.nodes_by_short_id:
            old_node = old.nodes_by_short_id[short_id]
            if isinstance(new_node, dict) and isinstance(old_node, dict):
                match_groups(matched, added, old_matched, new_node, old_node)
            elif isinstance(new_node, dict):
                match_groups(matched, added, old_matched, new_node, { old_node.file: old_node })
            elif isinstance(old_node, dict):
                match_groups(matched, added, old_matched, { new_node.file: new_node }, old_node)
            else:
                matched.append((new_node, old_node))
                old_matched.add(old_node)
        else:
            if isinstance(new_node, dict):
                for n in new_node.values():
                    added.append(n)
            else:
                added.append(new_node)

    deleted = list(set(old.nodes) - old_matched)

    changes:'list[AnyChange]' = []

    for node in deleted:
        changes.extend(get_add_delete_change(node, DELETED))

    for node in added:
        changes.extend(get_add_delete_change(node, ADDED))

    for nodes in matched:
        changes.extend(get_modification_changes(nodes[0], nodes[1]))

    return changes


class CompareResult:
    changes: 'list[AnyChange]'
    groups: 'list[GroupChanges]'
    files: 'list[FileChanges]'


def sort_changes(result: CompareResult):
    result.changes.sort(key=lambda x: (x.new.file, x.new.line))
    result.files.sort(key=lambda x: x.name)
    result.groups.sort(key=lambda x: x.name)
    for file in result.files:
        file.changes.sort(key=lambda x: x.new.line)
        file.groups.sort(key=lambda x: x.name)
        for group in file.groups:
            group.changes.sort(key=lambda x: x.new.line)
    for group in result.groups:
        group.changes.sort(key=lambda x: (x.new.file, x.new.line))
        group.files.sort(key=lambda x: x.name)
        for file in group.files:
            file.changes.sort(key=lambda x: x.new.line)


def compare(new: ParseResult, old: ParseResult) -> CompareResult:
    groups: 'dict[str, GroupChanges]' = {}
    groups_in_files: 'dict[str, GroupChanges]' = {}
    files: 'dict[str, FileChanges]' = {}
    files_in_groups: 'dict[str, FileChanges]' = {}
    changes = compare_nodes(new, old)
    for change in changes:
        node: Node = change.new
        group: 'Group | None' = None
        for parent_id in (node.parent_ids or []):
            parent = None
            if parent_id in new.nodes_by_id:
                parent = new.nodes_by_id[parent_id]
            if parent_id in old.nodes_by_id:
                parent = old.nodes_by_id[parent_id]
            if parent and isinstance(parent, Group):
                group = parent
        file_name = node.file
        group_name = group.name if group else ''
        combined_name = f'{file_name}```{group_name}'

        if file_name in files:
            file_changes = files[file_name]
        else:
            file_changes = FileChanges(file_name)
            files[file_name] = file_changes
        file_changes.changes.append(change)

        if group_name in groups:
            group_changes = groups[group_name]
        else:
            group_changes = GroupChanges(group_name, group.title if group else 'Unassigned')
            groups[group_name] = group_changes
        group_changes.changes.append(change)

        if combined_name in files_in_groups:
            file_in_group_changes = files_in_groups[combined_name]
            group_in_file_changes = groups_in_files[combined_name]
        else:
            file_in_group_changes = FileChanges(file_name)
            group_in_file_changes = GroupChanges(group_name, group.title if group else 'Unassigned')
            files_in_groups[combined_name] = file_in_group_changes
            groups_in_files[combined_name] = group_in_file_changes
            group_changes.files.append(file_in_group_changes)
            file_changes.groups.append(group_in_file_changes)
        file_in_group_changes.changes.append(change)
        group_in_file_changes.changes.append(change)

    result = CompareResult()
    result.changes = changes
    result.files = list(files.values())
    result.groups = list(groups.values())

    sort_changes(result)

    return result
