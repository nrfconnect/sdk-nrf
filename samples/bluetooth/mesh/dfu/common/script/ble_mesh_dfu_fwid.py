#!/usr/bin/env python3
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import sys
import os
import traceback
import argparse

def exit_with_error_msg():
    traceback.print_exc()
    print("Extracting BLE Mesh DFU FWID failed")
    sys.exit(0)

class KConfig(dict):

    @classmethod
    def from_file(cls, filename):
        """
        Extracts content of '.config' file into a dictionary

        Parameters:
            config_path (str): Path to '.config' file used by a firmware
        Returns:
            cls: A KConfig object where keys are Kconfig option names and values
            are option values, parsed from the config file at config_path.
        """
        configs = cls()
        try:
            with open(filename, 'r') as config:
                for line in config:
                    if not line.startswith("CONFIG_"):
                        continue
                    kconfig, value = line.split("=", 1)
                    configs[kconfig] = value.strip()
                return configs
        except Exception as err :
            raise Exception("Unable to parse .config file") from err

    def fwid_bytestring_get(self):
        try:
            clean_str = self['CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION'].replace("+", ".").replace("\"", "")
            version_list = [int(s) for s in clean_str.split(".") if s.isdigit()]

            fwid = bytearray()
            fwid.append(version_list[0])
            fwid.append(version_list[1])
            fwid.extend(version_list[2].to_bytes(2, 'little'))
            fwid.extend(version_list[3].to_bytes(4, 'little'))

            return str(fwid.hex())
        except Exception as err :
            raise Exception("Unable to parse CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION Kconfig option") from err

def input_parse():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument('--bin-path', required=True, type=str)
    return parser.parse_known_args()[0]

if __name__ == "__main__":
    try:
        args = input_parse()
        config_path = os.path.abspath(os.path.join(args.bin_path, '.config'))
        kconfigs = KConfig.from_file(config_path)

        print("Bluetooth Mesh Firmware ID generated:")
        print(f"\tFWID: {kconfigs.fwid_bytestring_get()}")

    except Exception:
        exit_with_error_msg()
