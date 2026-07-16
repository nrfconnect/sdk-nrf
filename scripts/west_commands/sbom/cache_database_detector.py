#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Detector that retrieves license and copyright information from a cache database.
'''

import json

from args import args
from common import SbomException
from data_structure import Data
from west import log

CACHE_SCHEMA_VERSION = 2


def detect(data: Data, optional: bool):
    '''Retrieve license and copyright information from the provided database.'''
    if args.input_cache_database is None:
        raise SbomException('No input cache database file.')

    with open(args.input_cache_database) as fd:
        log.dbg(f'Loading cache database from {args.input_cache_database}')
        db = json.load(fd)
    schema_version = db.get('schema_version', 1)
    if schema_version not in (1, CACHE_SCHEMA_VERSION):
        raise SbomException(f'Unsupported cache database schema version: {schema_version}')

    for file in data.files:
        key = str(file.file_rel_path)
        if key not in db['files']:
            continue
        entry = db['files'][key]
        if file.sha1 != entry['sha1']:
            continue

        licenses = set(entry.get('license', []))
        licenses_in_file = set(entry.get('license_in_file', []))
        copyright_texts = {
            text.strip()
            for text in entry.get('copyright', [])
            if isinstance(text, str) and text.strip()
        }

        # Legacy cache files have no license_in_file field. Their merged
        # license evidence cannot safely be classified as originating in-file.
        file.licenses.update(licenses)
        file.licenses.update(licenses_in_file)
        file.licenses_in_file.update(licenses_in_file)
        file.copyright_texts.update(copyright_texts)
        if licenses or licenses_in_file or copyright_texts:
            file.detectors.add('cache-database')
