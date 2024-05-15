# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from pathlib import Path
from compare import AnyChange, CompareResult
from jinja2 import Template
from args import args


def compile_messages(messages):
    result = {}
    for key in list(messages.keys()):
        message = messages[key]
        if message.startswith('ignore'):
            level = 0
        elif message.startswith('notice'):
            level = 1
        elif message.startswith('warning'):
            level = 2
        elif message.startswith('critical'):
            level = 3
        else:
            raise ValueError(f'Unknown level of message: {message}')
        result[key] = (Template(messages[key]), level)
    return result


messages: 'dict[str, tuple[Template, int]]' = compile_messages({
    'typedef-added': 'ignore',
    'typedef-deleted': 'critical: Type "{{old.name}}" definition deleted.',
    'typedef-modified-file': 'warning: Type "{{new.name}}" definition moved to a different file.',
    'typedef-modified-desc': 'notice: Type "{{new.name}}" definition description changed.',
    'typedef-modified-type': 'warning: Type "{{new.name}}" definition changed.',
    'var-added': 'ignore',
    'var-deleted': 'critical: Variable "{{old.name}}" deleted.',
    'var-modified-file': 'warning: Variable "{{new.name}}" moved to a different file.',
    'var-modified-desc': 'notice: Variable "{{new.name}}" description changed.',
    'var-modified-type': 'warning: Variable "{{new.name}}" type changed.',
    'enum_value-added': 'ignore',
    'enum_value-deleted': 'critical: Enum value "{{old.name}}" deleted.',
    'enum_value-modified-value': 'warning: Enum value "{{new.name}}" changed.',
    'enum_value-modified-desc': 'notice: Enum value "{{new.name}}" description changed.',
    'enum_value-modified-file': 'warning: Enum value "{{new.name}}" moved to a different file.',
    'enum-added': 'ignore',
    'enum-deleted': 'critical: Enum "{{old.name}}" deleted.',
    'enum-modified-file': 'warning: Enum "{{new.name}}" moved to a different file.',
    'enum-modified-desc': 'notice: Enum "{{new.name}}" description changed.',
    'struct-added': 'ignore',
    'struct-deleted': 'critical: Structure "{{old.name}}" deleted.',
    'struct-modified-file': 'warning: Structure "{{new.name}}" moved to a different file.',
    'struct-modified-desc': 'notice: Structure "{{new.name}}" description changed.',
    'func-added': 'ignore',
    'func-deleted': 'critical: Function "{{old.name}}" deleted.',
    'func-modified-return_type': 'warning: Function "{{new.name}}" return type changed.',
    'func-modified-file': 'warning: Function "{{new.name}}" moved to a different file.',
    'func-modified-desc': 'notice: Function "{{new.name}}" description changed.',
    'def-added': 'ignore',
    'def-deleted': 'critical: Definition "{{old.name}}" deleted.',
    'def-modified-value': 'notice: Definition "{{new.name}}" value changed.',
    'def-modified-file': 'warning: Definition "{{new.name}}" moved to a different file.',
    'def-modified-desc': 'notice: Definition "{{new.name}}" description changed.',
    'field-added': 'ignore',
    'field-deleted': 'critical: Structure "{{struct.new.name}}" field "{{new.name}}" deleted.',
    'field-modified-index': 'ignore',
    'field-modified-type': 'warning: Structure "{{struct.new.name}}" field "{{new.name}}" type changed.',
    'field-modified-desc': 'notice: Structure "{{struct.new.name}}" field "{{new.name}}" description changed.',
    'param-added': 'critical: Parameter "{{new.name}}" added in "{{parent.new.name}}".',
    'param-deleted': 'critical: Parameter "{{old.name}}" deleted from "{{parent.new.name}}".',
    'param-modified-index': 'critical: Parameter "{{new.name}}" reordered in "{{parent.new.name}}".',
    'param-modified-type': 'warning: Parameter "{{new.name}}" type changed in "{{parent.new.name}}".',
    'param-modified-desc': 'notice: Parameter "{{new.name}}" description changed in "{{parent.new.name}}".',
})


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
    if args.resolve_paths is not None:
        file = args.resolve_paths.joinpath(file).absolute()
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
                     location: 'tuple[Path, int | None]', **kwargs):
    for change in changes:
        prefix = f'{change.kind}-{change.action}'
        if change.new and hasattr(change.new, 'file') and change.new.file:
            if change.new and hasattr(change.new, 'line') and change.new.line:
                loc = (Path(change.new.file), change.new.line)
            else:
                loc = (Path(change.new.file), None)
        else:
            loc = location
        for key, (template, level) in messages.items():
            if key.startswith(prefix) and (level > 0):
                data = {}
                for name in dir(change):
                    value = getattr(change, name)
                    if (not callable(value)) and (not name.startswith('_')):
                        data[name] = value
                for name, value in kwargs.items():
                    data[name] = value
                message = template.render(**data)
                if key == prefix:
                    show_message(loc[0], loc[1], message, level)
                    stats[level] += 1
                else:
                    field = key[len(prefix) + 1:]
                    value = getattr(change, field)
                    if value:
                        show_message(loc[0], loc[1], message, level)
                        stats[level] += 1
        if prefix == 'struct-modified':
            generate_changes(stats, change.fields, loc, struct=change)
        elif prefix in ('func-modified', 'def-modified'):
            generate_changes(stats, change.params, loc, parent=change)


def generate(compare_result: CompareResult) -> 'list[int]':
    stats = [0, 0, 0, 0]
    for group in compare_result.groups:
        if group.name:
            print(f'=== Group {group.name}: {group.title} ===')
        generate_changes(stats, group.changes, (Path(), None))
    return stats
