#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Generates report using the Jinja2 templates.
'''

import os
import hashlib
from pathlib import Path
from typing import Any
from urllib.parse import quote
from jinja2 import Template, filters
from west import log
from data_structure import Data, FileInfo  # pylint: disable=unused-import
                                           # Ignoring false warning from pylint, FileInfo is used.


def rel_to_this_file(file_path: Path) -> Path:
    '''Convert a path to a relative version'''
    return './' + os.path.relpath(file_path, os.getcwd())


def verification_code(files: 'list[FileInfo]') -> str:
    '''Calculate verification code'''
    files.sort(key=lambda f: f.sha1)
    sha1 = hashlib.sha1()
    for file in files:
        sha1.update(file.sha1.encode('utf-8'))
    return sha1.hexdigest()

def adjust_identifier(license: str) -> str:
    '''Adjust LicenseRef identifier'''
    return license.replace('LICENSEREF', 'LicenseRef')

filters.FILTERS['rel_to_this_file'] = rel_to_this_file
filters.FILTERS['verification_code'] = verification_code
filters.FILTERS['adjust_identifier'] = adjust_identifier


def data_to_dict(data: Any) -> dict:
    '''Convert object to dict by copying public attributes to a new dictionary.'''
    result = dict()
    for name in dir(data):
        if name.startswith('_'):
            continue
        result[name] = getattr(data, name)
    return result


def generate(data: Data, output_file: 'Path|str', template_file: Path):
    '''Generate output_file from data using template_file.'''
    log.dbg(f'Writing output to "{output_file}" using template "{template_file}"')
    with open(template_file, 'r') as fd:
        template_source = fd.read()
    t = Template(template_source)
    out = t.render(**data_to_dict(data))
    with open(output_file, 'w') as fd:
        fd.write(out)
    escaped_path = quote(str(Path(output_file).resolve()).replace(os.sep, '/').strip("/"))
    log.inf(f'Output written to file:///{escaped_path}')
