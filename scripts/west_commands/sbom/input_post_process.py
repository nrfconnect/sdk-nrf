#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Post-processing of input files.
'''

import hashlib
from data_structure import Data, FileInfo, Package
from common import SbomException


def remove_duplicates(data: Data):
    '''Remove duplicated entries in data.files list.'''
    def is_not_visited(file: FileInfo):
        if file.file_path in visited:
            return False
        visited.add(file.file_path)
        return True
    visited = set()
    data.files = list(filter(is_not_visited, data.files))


def calculate_hashes(data: Data):
    '''Calculate SHA-1 hash for each entry in data.files list.'''
    for file in data.files:
        sha1 = hashlib.sha1()
        try:
            with open(file.file_path, 'rb') as fd:
                while True:
                    data = fd.read(65536)
                    if len(data) == 0 or data is None:
                        break
                    sha1.update(data)
        except Exception as ex:
            raise SbomException(f'Cannot calculate SHA-1 of the file "{file.file_path}"') from ex
        file.sha1 = sha1.hexdigest()


def post_process(data: Data):
    '''Post process input files by removing duplicates and calculating SHA-1'''
    if len(data.files) == 0:
        raise SbomException('No input files.\nRun "west ncs-sbom --help" for usage details.')
    remove_duplicates(data)
    calculate_hashes(data)
    data.packages[''] = Package()
