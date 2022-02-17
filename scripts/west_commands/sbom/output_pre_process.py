#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Pre-processing of data before it goes to the output.
'''

from data_structure import Data, License, LicenseExpr
from license_utils import get_license, get_spdx_license_expr_info, is_spdx_license


def pre_process(data: Data):
    '''Do pre-processing of data for simpler usage by the output modules.'''
    # Sort files
    data.files.sort(key=lambda f: f.file_path)
    # Convert list of licenses to license expression for each file
    for file in data.files:
        licenses = file.licenses if len(file.licenses) > 0 else ['']
        simple_expr_items = set()
        or_expr_items = set()
        repeated_expr_items = set()
        for license in licenses:
            license = license.upper()
            info = get_spdx_license_expr_info(license)
            if info.valid and not info.is_id_only and len(info.licenses) > 1:
                repeated_expr_items.update(info.licenses)
            if not info.valid or info.or_present:
                or_expr_items.add(license)
            else:
                simple_expr_items.add(license)
        simple_expr_items -= repeated_expr_items
        if len(or_expr_items) > 1 or len(simple_expr_items) > 0:
            or_expr_items = { f'({x})' for x in or_expr_items }
        file.license_expr = ' AND '.join(sorted(simple_expr_items.union(or_expr_items)))
    # Collect all used detectors
    for file in data.files:
        data.detectors.update(file.detectors)
    # Collect all used licenses and license expressions and put them into data.licenses
    used_licenses = dict()
    for file in data.files:
        if file.license_expr in used_licenses:
            continue
        info = get_spdx_license_expr_info(file.license_expr)
        if not info.valid or not info.is_id_only:
            expr = LicenseExpr()
            expr.valid = info.valid
            expr.id = file.license_expr
            expr.friendly_id = info.friendly_expr
            expr.licenses = sorted(info.licenses)
            expr.custom = not info.valid
            for id in expr.licenses:
                if not is_spdx_license(id):
                    expr.custom = True
                    break
            used_licenses[file.license_expr] = expr
        for id in info.licenses:
            if id in used_licenses:
                continue
            elif is_spdx_license(id):
                used_licenses[id] = get_license(id)
            elif id in data.licenses:
                used_licenses[id] = data.licenses[id]
            else:
                lic = get_license(id)
                if lic is None:
                    lic = License()
                    lic.id = id
                    lic.friendly_id = id
                used_licenses[id] = lic
    data.licenses = used_licenses
    # Group files by license id or expression
    for file in data.files:
        if file.license_expr not in data.files_by_license:
            data.files_by_license[file.license_expr] = list()
        data.files_by_license[file.license_expr].append(file)
    # Sort licenses
    def lic_reorder(id: str):
        lic = data.licenses[id]
        lic: 'License|LicenseExpr'
        if id == '':
            return 'A'
        elif lic.is_expr:
            if lic.custom:
                return 'B' + id
            else:
                return 'E' + id
        else:
            if id.startswith('LicenseRef-'):
                return 'D' + id
            elif lic.custom:
                return 'C' + id
            else:
                return 'F' + id
    data.licenses_sorted = sorted(data.licenses.keys(), key=lic_reorder)
