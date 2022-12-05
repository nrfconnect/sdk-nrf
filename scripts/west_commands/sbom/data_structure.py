#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
File contains classes that describes common data structure passed between elements
the west ncs-sbom command.
'''

import copy
from pathlib import Path
from uuid import uuid4


class DataBaseClass:
    '''Class that do shallow copy of class variables to instance variables.'''
    def __init__(self) -> None:
        for name in tuple(dir(self)):
            if name.startswith('_'):
                continue
            setattr(self, name, copy.copy(getattr(self, name)))


class FileInfo(DataBaseClass):
    ''' Contains file information about each input file
    Attributes:
        file_path           File path
        file_rel_path       Path relative to west workspace
        licenses            Set of detected SPDX license IDs and expressions
        license_expr        Contains SPDX license expression that covers all detected licesens
        package             Package ID of this file
        local_modifications The file was modified and does not match version of this package
        sha1                SHA-1 of the file
        detectors           Set of detectors that contributed to the list of licenses
    '''
    file_path: Path
    file_rel_path: Path
    licenses: 'set[str]' = set()
    license_expr: str
    package: 'str' = ''
    local_modifications: bool = False
    sha1: str
    detectors: 'set[str]' = set()


class License(DataBaseClass):
    ''' Contains license information
    Attributes:
        is_expr        Does this object contains license expression, always False for this class.
        id             Upper case license identifier
        friendly_id    User friendly license identifier (mixed letter case)
        custom         If this is a custom (not covered by SPDX standard) license
        name           User friendly name of the license
        url            URL to the on-line license information
        text           Full license text
        detectors      List of detector names that were involved into detecting licenses
    '''
    is_expr: bool = False
    id: str
    friendly_id: str
    custom: bool = True
    name: 'str|None' = None
    url: 'str|None' = None
    text: 'str|None' = None
    detectors: 'set[str]' = set()


class Package(DataBaseClass):
    ''' Contains package information
    Attributes:
        id             ID of this package
        name           User friendly name
        url            URL pointing to the source of this package
        version        Version string of this package
        browser_url    URL that can be opened in a web browser
    '''
    id: str = ''
    name: 'str|None' = None
    url: 'str|None' = None
    version: 'str|None' = None
    browser_url: 'str|None' = None


class LicenseExpr(DataBaseClass):
    ''' Contains license expression information
    Attributes:
        is_expr        Does this object contains license expression, always True for this class.
        id             Normalized upper case license expression
        friendly_id    User friendly license expression (mixed letter case)
        custom         True, if this any of the licenses used in this expression is a custom (not
                       covered by SPDX standard) license
        valid          True, if the expression can be correctly parsed
        licenses       List of all licenses used in this expression
    '''
    is_expr: bool = True
    id: str
    friendly_id: str
    custom: bool = True
    valid: bool
    licenses: 'list[str]' = list()


class Data(DataBaseClass):
    ''' Root data object passed by the script pipeline.

    It is created once at the beginning and passed over a pipeline:
    INPUT -> INPUT -> ... -> INPUT_POST_PROCESS -> DETECTOR -> DETECTOR -> ... ->
    -> OUTPUT_PRE_PROCESS -> OUTPUT -> OUTPUT ...

    Attributes:
        files            List of files. Inputs are responsible for filling this list. Input
                         post-process will remove duplicates. Detectors are responsible for
                         detecting license for each file.
        licenses         Dictionary of all licenses and expression. Key is normalized (upper case)
                         identifier of the license or the license expression.
        licenses_sorted  Sorted list of licenses. Output should contain licenses in that order.
        packages         Dictionary of all packages. Key is package ID. It contains special
                         package with empty ID string, which indicates unknown package.
        packages_sorted  Sorted list of packages. Output should contain packages in that order.
        inputs           List of user friendly input description.
        detectors        Set containing all detectors that were involved in license detection.
        report_uuid      Random UUID that can be used in the output.
    '''
    files: 'list[FileInfo]' = list()
    licenses: 'dict[License|LicenseExpr]' = dict()
    licenses_sorted: 'list[str]' = list()
    packages: 'dict[Package]' = dict()
    packages_sorted: 'list[str]' = list()
    inputs: 'list[str]' = list()
    detectors: 'set[str]' = set()
    report_uuid: 'str' = uuid4()
