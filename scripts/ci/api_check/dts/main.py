# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import os
import sys
import json
import out_text
from json import JSONEncoder
from pathlib import Path
from dts_tools import error
from args import args
from bindings_parser import parse_bindings, save_bindings
from dts_compare import compare


def collect_zephyr_inputs() -> 'list[Path]':
    zephyr_base = os.getenv('ZEPHYR_BASE')
    if zephyr_base is None:
        error('You must specify "ZEPHYR_BASE" if you using "-" argument as an input.')
        sys.exit(1)
    zephyr_base = Path(zephyr_base)
    west_root = zephyr_base.parent
    bindings_dirs: 'list[Path]' = []
    for bindings_dir in west_root.glob('**/dts/bindings'):
        rel = bindings_dir.relative_to(west_root)
        if str(rel).count('test') or str(rel).count('sample'):
            continue
        bindings_dirs.append(bindings_dir)
    return bindings_dirs


def collect_inputs(arguments: 'list[list[str]]') -> 'list[Path]|Path':
    result_list: 'list[Path]' = []
    result_pickle: 'Path|None' = None
    for arg_list in arguments:
        for arg in arg_list:
            if arg == '-':
                result_list.extend(collect_zephyr_inputs())
            else:
                arg = Path(arg)
                if arg.is_file():
                    result_pickle = arg
                else:
                    result_list.append(arg)
    if len(result_list) > 0 and result_pickle is not None:
        error('Expecting pickled file or list of directories. Not both.')
        sys.exit(1)
    return result_pickle or result_list


def dump_json(file: Path, **kwargs):
    def default_encode(o):
        this_id = id(o)
        if this_id in ids:
            return f'__id__{this_id}'
        if isinstance(o, set):
            return list(o)
        else:
            ids.add(this_id)
            d = {'__id__': f'__id__{this_id}'}
            for name in tuple(dir(o)):
                if not name.startswith('_'):
                    value = getattr(o, name)
                    if not callable(value):
                        d[name] = value
            return d
    ids = set()
    with open(file, 'w') as fd:
        fd.write('{\n')
        first = True
        for name, value in kwargs.items():
            json = JSONEncoder(sort_keys=False, indent=2, default=default_encode).encode(value)
            if not first:
                fd.write(',\n')
            fd.write(f'"{name}": {json}')
            first = False
        fd.write('\n}\n')

def main():
    new_input = parse_bindings(collect_inputs(args.new))

    if args.old:
        old_input = parse_bindings(collect_inputs(args.old))
    else:
        old_input = None

    if args.save_input:
        save_bindings(new_input, args.save_input)

    if args.save_old_input and old_input:
        save_bindings(old_input, args.save_old_input)

    if args.dump_json:
        if old_input:
            dump_json(args.dump_json,
                      new_bindings=new_input.bindings,
                      new_binding_by_name=new_input.binding_by_name,
                      old_bindings=old_input.bindings,
                      old_binding_by_name=old_input.binding_by_name)
        else:
            dump_json(args.dump_json,
                      bindings=new_input.bindings,
                      binding_by_name=new_input.binding_by_name)

    level = 0

    if old_input:
        changes = compare(new_input, old_input)
        if args.dump_json:
            dump_json(args.dump_json,
                      new_bindings=new_input.bindings,
                      new_binding_by_name=new_input.binding_by_name,
                      old_bindings=old_input.bindings,
                      old_binding_by_name=old_input.binding_by_name,
                      changes=changes)
        stats = out_text.generate(changes)
        if args.save_stats:
            args.save_stats.write_text(json.dumps({
                'notice': stats[1],
                'warning': stats[2],
                'critical': stats[3],
            }, indent=2))
        for i, count in enumerate(stats):
            if count > 0:
                level = i

    sys.exit(level)


if __name__ == '__main__':
    main()
