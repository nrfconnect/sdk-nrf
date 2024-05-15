# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import re
from pathlib import Path
from jinja2 import Template
from dts_compare import AnyChange, BindingChange
from dts_tools import compile_messages
from args import args


spaces_re = re.compile(r'\s+')


messages: 'dict[str, Template]' = compile_messages({
    'binding-added': 'ignore',
    'binding-deleted': 'critical: Binding "{{new.name}}" deleted.',
    'binding-modified-path': 'ignore',
    'binding-modified-description': 'notice: Binding "{{new.name}}" description changed.',
    'binding-modified-cells': 'notice: Binding "{{new.name}}" cells definition changed from "{{old.cells}}" to "{{new.cells}}".',
    'binding-modified-buses': 'warning: Binding "{{new.name}}" buses definition changed from "{{old.buses}}" to "{{new.buses}}".',
    'property-added': '''
        {% if new.const is not none %}
            ignore
        {% elif new.default is not none %}
            notice: Property "{{new.name}}" of "{{binding.new.name}}" added with default value.
        {% elif not new.required %}
            warning: Property "{{new.name}}" of "{{binding.new.name}}" added, but it is not required.
        {% else %}
            critical: Required property "{{new.name}}" of "{{binding.new.name}}" added.
        {% endif %}
        ''',
    'property-deleted': '''
        {% if new.deprecated %}
            notice: Deprecated property "{{new.name}}" of "{{binding.new.name}}" deleted.
        {% else %}
            critical: Property "{{new.name}}" of "{{binding.new.name}}" deleted.
        {% endif %}
        ''',
    'property-modified-type': '{% if new.const is not none %} critical: Property "{{new.name}}" of "{{binding.new.name}}" type changed. {% endif %}',
    'property-modified-description': '{% if new.const is not none %} notice: Property "{{new.name}}" of "{{binding.new.name}}" description changed. {% endif %}',
    'property-modified-const-added': 'critical: Property "{{new.name}}" of "{{binding.new.name}}" const value set.',
    'property-modified-const-deleted': '''
        {% if new.default is none %}
            critical: Property "{{new.name}}" of "{{binding.new.name}}" const value removed.
        {% else %}
            notice: Property "{{new.name}}" of "{{binding.new.name}}" const value replaced by default value.
        {% endif %}
        ''',
    'property-modified-const-modified': 'ignore',
    'property-modified-default-added': 'ignore',
    'property-modified-default-deleted': 'critical: Property "{{new.name}}" of "{{binding.new.name}}" default value removed.',
    'property-modified-default-modified': 'critical: Property "{{new.name}}" of "{{binding.new.name}}" default value modified.',
    'property-modified-deprecated-set': 'ignore',
    'property-modified-deprecated-cleared': 'ignore',
    'property-modified-required-set': 'critical: Property "{{new.name}}" of "{{binding.new.name}}" is now required.',
    'property-modified-required-cleared': 'ignore',
    'property-modified-specifier_space': '{% if new.const is not none %} warning: Property "{{new.name}}" of "{{binding.new.name}}" specifier space changed. {% endif %}',
    'enum-added': 'ignore',
    'enum-deleted': 'critical: Enum value "{{new}}" of property "{{property.new.name}}" of "{{binding.new.name}}" deleted.',
})


def get_message_level(message: str) -> int:
    if message.startswith('ignore') or (message == ''):
        return 0
    elif message.startswith('notice'):
        return 1
    elif message.startswith('warning'):
        return 2
    elif message.startswith('critical'):
        return 3
    else:
        raise ValueError(f'Unknown level of message: {message}')

github_commands = [
    '::ignore',
    '::notice',
    '::warning',
    '::error'
]

github_titles = [
    'Ignore',
    'Notice',
    'Warning',
    'Critical',
]

def encode(text: str, is_message: bool):
    if is_message:
        return text.replace('%', '%25').replace('\r', '%0D').replace('\n', '%0A')
    else:
        return text.replace('%', '%25').replace('\r', '%0D').replace('\n', '%0A').replace(',', '%2C').replace('::', '%3A%3A')

def show_message(file: Path, line: 'str | int | None', message: str, level: int):
    if args.relative_to is not None:
        file = file.relative_to(args.relative_to)
    if args.format == 'github':
        command = github_commands[level]
        title = f'Compatibility {github_titles[level]}'
        if line is not None:
            print(f'{command} file={encode(str(file), False)},line={line},title={title}::{encode(message, True)}')
        else:
            print(f'{command} file={encode(str(file), False)},title={title}::{encode(message, True)}')
    elif line is not None:
        print(f'{file}:{line}: {message}')
    else:
        print(f'{file}: {message}')


def generate_changes(stats: 'list[int]', changes: 'list[AnyChange]',
                     location: Path, **kwargs) -> int:
    for change in changes:
        loc = Path(change.new.path) if isinstance(change, BindingChange) else location
        prefix = f'{change.kind}-{change.action}'
        for key, template in messages.items():
            if not key.startswith(prefix):
                continue
            matched = False
            if key == prefix:
                matched = True
            else:
                parts = key[len(prefix) + 1:].split('-')
                field = parts[0]
                expected = parts[1] if (len(parts) > 1) else True
                value = getattr(change, field)
                if value == expected:
                    matched = True
            if not matched:
                continue
            data = {}
            for name in dir(change):
                value = getattr(change, name)
                if (not callable(value)) and (not name.startswith('_')):
                    data[name] = value
            for name, value in kwargs.items():
                data[name] = value
            message = spaces_re.sub(template.render(**data), ' ').strip()
            level = get_message_level(message)
            if level > 0:
                show_message(loc, None, message, level)
                stats[level] += 1
        if prefix == 'binding-modified':
            generate_changes(stats, change.properties, loc, binding=change)
        elif prefix == 'property-modified':
            generate_changes(stats, change.enum, loc, property=change, **kwargs)


def generate(compare_result: 'list[BindingChange]'):
    stats = [0, 0, 0, 0]
    generate_changes(stats, compare_result, Path())
    return stats
