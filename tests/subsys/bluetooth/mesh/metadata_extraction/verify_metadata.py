# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import json
import os
import sys
from zipfile import ZipFile


def assert_metadata_equal(json_data, expected, msg="Metadata not equal to expected"):
    """
    Check metadata instance for equality, printing diff on error
    """
    diff_lines = [msg]

    def list_diff(json, expected, path=''):
        if len(json) != len(expected):
            diff_lines.append(f"Expected {path} to have length {len(expected)} but got {len(json)}")
        else:
            for i, json_item in enumerate(json):
                diff(json_item, expected[i], f"{path}[{i}]")

    def dict_diff(json, expected, path=''):
        for key in set(json) | set(expected):
            full_path = path + key
            if key not in expected:
                diff_lines.append(f"Unexpected key {full_path} in JSON")
            elif key not in json:
                diff_lines.append(f"Key {full_path} missing from JSON")
            else:
                diff(json[key], expected[key], full_path)

    def diff(json, expected, path=''):
        if json != expected:
            if isinstance(json, dict) and isinstance(expected, dict):
                dict_diff(json, expected, f"{path}.")
            elif isinstance(json, list) and isinstance(expected, list):
                list_diff(json, expected, path)
            else:
                diff_lines.append(f"Expected {path} == {expected} but got {json}")

    if json_data != expected:
        dict_diff(json_data, expected)
        raise AssertionError('\n'.join(diff_lines))


def expected_metadata(size):
    encoded_size = size.to_bytes(3, 'little').hex()
    return [
        {
            'sign_version': {'major': 1, 'minor': 2, 'revision': 3, 'build_number': 4},
            'binary_size': size,
            'core_type': 1,
            'composition_data': {
                'cid': 1,
                'pid': 2,
                'vid': 3,
                'crpl': 64,
                'features': 5,
                'elements': [
                    {'location': 1, 'sig_models': [0, 2], 'vendor_models': []}
                ]
            },
            'composition_hash': '0x587a2fb',
            'encoded_metadata': f'0102030004000000{encoded_size}01fba287050100'
        },
        {
            'sign_version': {'major': 1, 'minor': 2, 'revision': 3, 'build_number': 4},
            'binary_size': size,
            'core_type': 1,
            'composition_data': {
                'cid': 4,
                'pid': 5,
                'vid': 6,
                'crpl': 64,
                'features': 5,
                'elements': [
                    {'location': 1, 'sig_models': [0, 2], 'vendor_models': []},
                    {'location': 2, 'sig_models': [4097, 4099], 'vendor_models': []}
                ]
            },
            'composition_hash': '0x13f1a143',
            'encoded_metadata': f'0102030004000000{encoded_size}0143a1f1130200'
        }
    ]


if __name__ == '__main__':
    comp_data_layout = sys.argv[1]
    bin_dir = sys.argv[2]
    type = sys.argv[3]

    if type == "sysbuild":
        zip_path = os.path.join(bin_dir, "dfu_application.zip")
        binary_size = os.path.getsize(os.path.join(bin_dir, "metadata_extraction", "zephyr", "zephyr.signed.bin"))
    else:
        zip_path = os.path.join(bin_dir, "zephyr", "dfu_application.zip")
        binary_size = os.path.getsize(os.path.join(bin_dir, "zephyr", "app_update.bin"))

    expected = expected_metadata(binary_size)

    with ZipFile(zip_path) as zip_file:
        json_string = zip_file.read('ble_mesh_metadata.json').decode()

    json_data = json.loads(json_string)

    if comp_data_layout == '--single':
        assert not isinstance(json_data, list), "Expected single metadata but got multiple."
        assert_metadata_equal(json_data, expected[0])
    elif comp_data_layout == '--multiple':
        assert not isinstance(json_data, dict), "Expected multiple metadata but got one."
        assert len(json_data) == len(expected), (f"Expected {len(expected)} metadatas "
                                                          f"but got {len(json_data)}")
        for i, data in enumerate(json_data):
            assert_metadata_equal(data, expected[i], f"Metadata[{i}] not equal to expected")
    else:
        raise ValueError('Unknown comp data layout')
