#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Detector that detect license using provided cache database file.
'''

import json
from west import log
from args import args
from data_structure import Data
from common import SbomException


def detect(data: Data, optional: bool):
    '''Retrieve the license from the provided database.'''
    if args.input_cache_database is None:
        raise SbomException('No input cache database file.')

    with open(args.input_cache_database, 'r') as fd:
        log.dbg(f'Loading cache database from {args.input_cache_database}')
        db = json.load(fd)

    for file in data.files:
        if optional and file.licenses:
            continue
        key = str(file.file_rel_path)
        if key not in db['files']:
            continue
        if file.sha1 == db['files'][key]['sha1']:
            file.licenses = db['files'][key]['license']
        file.detectors.add('cache-database')
