#!/usr/bin/env python3
#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import errno
import argparse
import json
import time
import yaml
from pathlib import Path, PurePath
from zipfile import ZipFile
from os import path


def parse_args():
    parser = argparse.ArgumentParser(
        description="Generate zip file for binary artifact. Provide a list of 'key=value' pairs separated by space for"
                    "information that should be stored in manifest.json. To provide information specific for one of "
                    "the files, prepend the 'key' with the name of the file (only the basename, not the path)",
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument('--output', required=True, type=argparse.FileType(mode='w'), help='Output zip path')
    parser.add_argument('--bin-files', required=True, type=argparse.FileType(mode='r'), nargs='+',
                        help='Bin files to be stored in zip')
    parser.add_argument('--meta-info-file', required=False,
                        help='''File containg build meta info for an nRF Connect
                        SDK build. The Zephyr and nRF Connect SDK revisions
                        will be fetched from the build meta info file and
                        append to the manifest content. The format of the file
                        must follow the structure of the Zephyr build info meta
                        file.''')
    parser.add_argument('--name', required=False, help='Optional name to display to the user to help identify the '
                        'purpose of this update')
    return parser.parse_known_args()


def get_name(text_wrapper):
    return path.basename(text_wrapper.name)


if __name__ == '__main__':
    args, info = parse_args()

    manifest = {
        'format-version': 0,
        'time': int(time.time()),
        'files': list()
    }

    if args.name is not None:
        manifest['name'] = args.name

    if args.meta_info_file is not None:
        meta = None
        meta_file = PurePath(args.meta_info_file)
        if Path(meta_file).is_file():
            with Path(meta_file).open('r') as f:
                meta = yaml.safe_load(f.read())
        else:
            raise OSError(errno.ENOENT, "Meta info file not found", args.meta_info_file)

        if meta is not None:
            zephyr_revision = meta.get('zephyr', {}).get('revision')
            nrf_revision = None

            for module in meta.get('modules', []):
                if module.get('name') == 'nrf':
                    nrf_revision = module.get('revision')
                    break

            firmware_revisions = {'zephyr': {'revision': zephyr_revision},
                                  'nrf': {'revision': nrf_revision}}
            manifest['firmware'] = firmware_revisions

    shared_info = dict()
    special_info = dict()
    name_to_path = {get_name(f): f.name for f in args.bin_files}

    for i in info:
        special = False
        key = i.split('=')[0]
        val = i.split('=')[1]
        if val.startswith('0x'):
            val = int(val, base=16)

        # When multiple bin files are given, the non-shared configurations are prepended with the name of the bin file
        for p in name_to_path.keys():
            if p not in special_info:
                special_info[p] = dict()
            if key.startswith(p):
                special_info[p][key.replace(p, '')] = val
                special = True
        if not special:
            shared_info[key] = val

    for n, p in name_to_path.items():
        special_info[n]['size'] = path.getsize(p)
        special_info[n]['file'] = n
        special_info[n]['modtime'] = int(path.getmtime(p))
        merged = dict()
        merged.update(shared_info)
        merged.update(special_info[n])
        manifest['files'].append(merged)

    path_to_manifest = path.join(path.dirname(args.output.name), f'{path.basename(args.output.name)}_manifest.json')
    with open(path_to_manifest, 'w') as manifest_file:
        manifest_file.write(json.dumps(manifest, indent=4))

    name_to_path['manifest.json'] = path_to_manifest

    with ZipFile(args.output.name, 'w') as my_zip:
        for n, p in name_to_path.items():
            my_zip.write(p, n)
