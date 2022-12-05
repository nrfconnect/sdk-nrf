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
from urllib.parse import quote
from datetime import datetime, timezone
from jinja2 import Template, filters
from west import log
from data_structure import Data, FileInfo  # pylint: disable=unused-import
                                           # Ignoring false warning from pylint, FileInfo is used.


counter_value = 0


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


filters.FILTERS['verification_code'] = verification_code
filters.FILTERS['adjust_identifier'] = adjust_identifier


def group_by(files: 'list[FileInfo]', attr_name: str) -> 'dict[list[FileInfo]]':
    result = dict()
    for file in files:
        attr_value = getattr(file, attr_name)
        if attr_value not in result:
            result[attr_value] = list()
        result[attr_value].append(file)
    return result


def counter() -> int:
    global counter_value
    counter_value += 1
    return counter_value


def relative_path(file: 'str|Path', output_directory: Path) -> str:
    return './' + os.path.relpath(Path(file).resolve(), output_directory)


def timestamp() -> str:
    return datetime.now(timezone.utc).isoformat(timespec='seconds').replace('+00:00', 'Z')


def data_to_dict(data: Data, output_directory: Path) -> dict:
    '''Convert object to dict by copying public attributes to a new dictionary.'''
    result = dict()
    for name in dir(data):
        if name.startswith('_'):
            continue
        result[name] = getattr(data, name)
    result['func'] = {
        'group_by': group_by,
        'counter': counter,
        'relative_path': lambda file: relative_path(file, output_directory),
        'timestamp': timestamp
    }
    return result


def generate(data: Data, output_file: 'Path|str', template_file: Path):
    '''Generate output_file from data using template_file.'''
    output_file = Path(output_file)
    log.dbg(f'Writing output to "{output_file}" using template "{template_file}"')
    with open(template_file, 'r') as fd:
        template_source = fd.read()
    t = Template(template_source)
    out = t.render(**data_to_dict(data, output_file.parent.resolve()))
    with open(output_file, 'w') as fd:
        fd.write(out)
    escaped_path = quote(str(output_file.resolve()).replace(os.sep, '/').strip("/"))
    log.inf(f'Output written to file:///{escaped_path}')
