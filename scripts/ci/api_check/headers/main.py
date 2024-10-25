# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import sys
import json
import out_text
from args import args
from compare import compare
from dox_parser import dump_doxygen_json, parse_doxygen, save_doxygen


def main():
    new_input = parse_doxygen(args.new_input)

    if args.old_input:
        old_input = parse_doxygen(args.old_input)
    else:
        old_input = None

    if args.save_input:
        save_doxygen(new_input, args.save_input)

    if args.save_old_input and old_input:
        save_doxygen(old_input, args.save_old_input)

    if args.dump_json:
        if old_input:
            dir = args.dump_json.parent
            name = args.dump_json.name
            suffix = args.dump_json.suffix
            dump_doxygen_json(new_input, dir / (name[0:-len(args.dump_json.suffix)] + '.new' + suffix))
            dump_doxygen_json(old_input, dir / (name[0:-len(args.dump_json.suffix)] + '.old' + suffix))
        else:
            dump_doxygen_json(new_input, args.dump_json)

    level = 0

    if old_input:
        changes = compare(new_input, old_input)
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
