# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from typing import Any
from bindings_parser import Binding, Property, ParseResult


UNCHANGED = 'unchanged'
ADDED = 'added'
DELETED = 'deleted'
MODIFIED = 'modified'
SET = 'set'
CLEARED = 'cleared'


class AnyChange:
    kind: str
    action: str
    def __init__(self, action: str, new: Any, old: Any):
        self.action = action
        self.new = new
        self.old = old


class BindingChange(AnyChange):
    kind = 'binding'
    new: Binding
    old: Binding
    path: bool = False
    description: bool = False
    cells: bool = False
    buses: bool = False
    properties: 'list[PropertyChange]'
    def __init__(self, action: str, new: Any, old: Any):
        super().__init__(action, new, old)
        self.properties = []


class PropertyChange(AnyChange):
    kind = 'property'
    new: Property
    old: Property
    type: bool = False
    description: bool = False
    enum: 'list[EnumChange]'
    const: str = UNCHANGED
    default: str = UNCHANGED
    deprecated: str = UNCHANGED
    required: str = UNCHANGED
    specifier_space: bool = False
    def __init__(self, action: str, new: Any, old: Any):
        super().__init__(action, new, old)
        self.enum = []


class EnumChange(AnyChange):
    kind = 'enum'
    new: str
    old: str


def get_str_action(new: 'str | None', old: 'str | None') -> str:
    if (new is None) and (old is None):
        return UNCHANGED
    elif (new is None) and (old is not None):
        return DELETED
    elif (new is not None) and (old is None):
        return ADDED
    else:
        return MODIFIED if new != old else UNCHANGED


def get_bool_action(new: bool, old: bool) -> str:
    if (not new) and old:
        return CLEARED
    elif new and (not old):
        return SET
    elif (new is not None) and (old is None):
        return UNCHANGED


def compare_properties(new: 'dict[str, Property]', old: 'dict[str, Property]') -> 'list[PropertyChange]':
    new_keys = set(new.keys())
    old_keys = set(old.keys())
    added_keys = new_keys.difference(old_keys)
    deleted_keys = old_keys.difference(new_keys)
    remaining_keys = new_keys.intersection(old_keys)
    result: 'list[PropertyChange]' = []
    for key in added_keys:
        property_change = PropertyChange(ADDED, new[key], new[key])
        result.append(property_change)
    for key in deleted_keys:
        property_change = PropertyChange(DELETED, old[key], old[key])
        result.append(property_change)
    for key in remaining_keys:
        new_property = new[key]
        old_property = old[key]
        property_change = PropertyChange(MODIFIED, new[key], old[key])
        property_change.type = new_property.type != old_property.type
        property_change.description = new_property.description != old_property.description
        property_change.const = get_str_action(new_property.const, old_property.const)
        property_change.default = get_str_action(new_property.default, old_property.default)
        property_change.deprecated = get_bool_action(new_property.deprecated, old_property.deprecated)
        property_change.required = get_bool_action(new_property.required, old_property.required)
        property_change.specifier_space = new_property.specifier_space != old_property.specifier_space
        for enum_value in new_property.enum.difference(old_property.enum):
            property_change.enum.append(EnumChange(ADDED, enum_value, enum_value))
        for enum_value in old_property.enum.difference(new_property.enum):
            property_change.enum.append(EnumChange(DELETED, enum_value, enum_value))
        changed = (
            property_change.type or
            property_change.description or
            property_change.const != UNCHANGED or
            property_change.default != UNCHANGED or
            property_change.deprecated != UNCHANGED or
            property_change.required != UNCHANGED or
            property_change.specifier_space or
            len(property_change.enum))
        if changed:
            result.append(property_change)
    return result


def compare(new: ParseResult, old: ParseResult) -> 'list[BindingChange]':
    new_keys = set(new.binding_by_name.keys())
    old_keys = set(old.binding_by_name.keys())
    added_keys = new_keys.difference(old_keys)
    deleted_keys = old_keys.difference(new_keys)
    remaining_keys = new_keys.intersection(old_keys)
    result: 'list[BindingChange]' = []
    for key in added_keys:
        binding_change = BindingChange(ADDED, new.binding_by_name[key], new.binding_by_name[key])
        result.append(binding_change)
    for key in deleted_keys:
        binding_change = BindingChange(DELETED, old.binding_by_name[key], old.binding_by_name[key])
        result.append(binding_change)
    for key in remaining_keys:
        new_binding = new.binding_by_name[key]
        old_binding = old.binding_by_name[key]
        binding_change = BindingChange(MODIFIED, new.binding_by_name[key], old.binding_by_name[key])
        binding_change.path = new_binding.path != old_binding.path
        binding_change.description = new_binding.description != old_binding.description
        binding_change.buses = new_binding.buses != old_binding.buses
        binding_change.cells = new_binding.cells != old_binding.cells
        binding_change.properties = compare_properties(new_binding.properties, old_binding.properties)
        changed = (binding_change.path or
            binding_change.description or
            binding_change.buses or
            binding_change.cells or
            len(binding_change.properties))
        if changed:
            result.append(binding_change)
    return result
