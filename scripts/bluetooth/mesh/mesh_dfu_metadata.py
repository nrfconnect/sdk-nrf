#!/usr/bin/env python3
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
This script is used to extract the Bluetooth Mesh composition data from the build folder of
an application. This information is used to generate a hash that can verify if the composition
of a device will change after a Mesh device firmware upgrade has completed.
The input to this script is the following content of the build folder of an application:
    - build/zephyr/zephyr.elf
    - build/zephyr/.config
The script produces the following output to the build folder:
	- build/zephyr/dfu_application.zip_mesh_metadata.json
This output is also appended to the archive located at build/zephyr/dfu_application.zip.
'''

import struct
import sys
import os
from elftools.elf.elffile import ELFFile
import json
from cryptography.hazmat.primitives import cmac
from cryptography.hazmat.primitives.ciphers import algorithms
from zipfile import ZipFile
import traceback
import argparse

FILE_NAME_IN_ZIP = 'ble_mesh_metadata.json'
FILE_NAME = 'dfu_application.zip_ble_mesh_metadata.json'


def exit_with_error_msg():
    traceback.print_exc()
    print("Extracting BLE Mesh metadata failed")
    print("You can bypass this script by disabling the CONFIG_BT_MESH_DFU_METADATA_ON_BUILD option in your project config")
    sys.exit(0)


class Elem:
    def __init__(self, loc):
        self.loc = loc
        self.vnd_list = []
        self.sig_list = []

    def vnd_model_add(self, cid, vid):
        self.vnd_list.append((cid << 16) + vid)

    def sig_model_add(self, id):
        self.sig_list.append(id)

    def bytestring_generate(self):
        bytestring = bytearray()
        bytestring.extend(self.loc.to_bytes(2, 'little'))
        bytestring.append(len(self.sig_list))
        bytestring.append(len(self.vnd_list))

        for sig in self.sig_list:
            bytestring.extend(sig.to_bytes(2, 'little'))
        for vnd in self.vnd_list:
            bytestring.extend(vnd.to_bytes(4, 'little'))

        return bytestring

    def dict_generate(self):
        return {
            "location": self.loc,
            "sig_models": self.sig_list,
            "vendor_models": self.vnd_list,
        }


class Comp0:
    # Must stay in order
    FEATURE_KCONF_OPTS = [
        'CONFIG_BT_MESH_RELAY',
        'CONFIG_BT_MESH_GATT_PROXY',
        'CONFIG_BT_MESH_FRIEND',
        'CONFIG_BT_MESH_LOW_POWER',
    ]

    def __init__(self, cid, pid, vid, kconfig):
        if 'CONFIG_BT_MESH_CRPL' not in kconfig.keys():
            raise Exception("Could not find CONFIG_BT_MESH_CRPL Kconfig option")
        self.elems = []
        self.cid = cid
        self.pid = pid
        self.vid = vid
        self.__features_add(kconfig)
        self.hash = None

    def elem_add(self, loc):
        new_elem = Elem(loc)
        self.elems.append(new_elem)
        return new_elem

    def __features_add(self, kconfig):

        self.feat = 0
        self.crpl = int(kconfig['CONFIG_BT_MESH_CRPL'])

        for i, opt in enumerate(self.FEATURE_KCONF_OPTS):
            self.feat += (1 if kconfig.get(opt) == 'y' else 0) << i

    def __bytestring_generate(self):
        bytestring = bytearray()
        bytestring.extend(self.cid.to_bytes(2, 'little'))
        bytestring.extend(self.pid.to_bytes(2, 'little'))
        bytestring.extend(self.vid.to_bytes(2, 'little'))
        bytestring.extend(self.crpl.to_bytes(2, 'little'))
        bytestring.extend(self.feat.to_bytes(2, 'little'))

        for elem in self.elems:
            bytestring.extend(elem.bytestring_generate())

        return bytestring

    def dict_generate(self):
        return {
            "cid": self.cid,
            "pid": self.pid,
            "vid": self.vid,
            "crpl": self.crpl,
            "features": self.feat,
            "elements": [e.dict_generate() for e in self.elems]
        }

    def hash_generate(self):
        # Uses 16-byte zero key
        crypto = cmac.CMAC(algorithms.AES(bytes(16)))
        crypto.update(bytes(self.__bytestring_generate()))
        self.hash, *_ = struct.unpack('<L', crypto.finalize()[:4])
        return self.hash


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

    def version_parse(self):
        try:
            clean_str = self['CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION'].replace("+", ".").replace("\"", "")
            version_list = [int(s) for s in clean_str.split(".") if s.isdigit()]
            return {
                "major": version_list[0],
                "minor": version_list[1],
                "revision": version_list[2],
                "build_number": version_list[3],
            }
        except Exception as err :
            raise Exception("Unable to parse CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION Kconfig option") from err


def read_symbol_data(elf, symbol_addr):
    """
    Reads variable data from the '.symtab' section of the .elf file.

    Parameters:
        elf (ELFFile): ELFFile instance
        symbol_addr (int): Address of the variable to read

    Returns:
        bytearray: Data read from the specified address
    """
    section = elf.get_section_by_name('.symtab')
    if section is None:
        raise Exception('Unable to find .symtab section')
    symbol = None
    for s in section.iter_symbols():
        if (s.entry.st_value == symbol_addr) and\
            (len(s.name) > 0) and\
            ("$" not in s.name) and\
            s.entry.st_size > 0:
            symbol = s
            break
    else:
        raise Exception('Unable to find symbol at address {symbol_addr}')
    file_offset = None
    for segment in elf.iter_segments():
        if segment.header['p_type'] != 'PT_LOAD':
            continue
        if (symbol['st_value'] >= segment['p_vaddr']) and\
            (symbol['st_value'] < segment['p_vaddr'] + segment['p_filesz']):
            file_offset = symbol['st_value'] - segment['p_vaddr'] + segment['p_offset']
            break
    else:
        raise Exception('Error getting file offset from ELF data')
    elf.stream.seek(file_offset)
    sz = symbol['st_size']

    return elf.stream.read(sz)

def find_comp_data_from_dwarf(elf_path):
    """
    Find all occurrences of the `bt_mesh_comp` variable in the .elf file

    The composition data declaration must have the const-qualifier. It can also be declared as an array. Example:
        ```
        static const struct bt_mesh_comp comp;
        const struct bt_mesh_comp comp;
        const struct bt_mesh_comp comp[2];
        ```
    Parameters:
        elf_path (ELFFile): ELFFile instance

    Returns:
        List(addr): Addresses of the found composition data instances in the firmware.
    """
    DW_OP_addr = 0x3

    with open(elf_path, 'rb') as file:
        elf_file = ELFFile(file)
        dwarf_info = elf_file.get_dwarf_info()
        comp_data_arr = []
        for cu_header in dwarf_info.iter_CUs():
            for die in cu_header.iter_DIEs():
                if die.tag != 'DW_TAG_variable':
                    continue
                location = die.attributes.get('DW_AT_location')
                if location is None:
                    continue
                if location.form not in ("DW_FORM_exprloc"):
                    continue
                opcode = location.value[0]
                if opcode != DW_OP_addr:
                    continue

                addr = int.from_bytes(die.attributes.get('DW_AT_location').value[1:5], 'little')

                if 'DW_AT_abstract_origin' in die.attributes and \
                    die.attributes.get('DW_AT_abstract_origin').form == 'DW_FORM_ref_addr':
                    # If address is moved to another die, find original variable through the
                    # reference and continue with the new die.
                    value = die.attributes.get('DW_AT_abstract_origin').value
                    die = dwarf_info.get_DIE_from_refaddr(value)
                    if die is None:
                        continue

                # Check that the variable type is either `const struct bt_mesh_comp` or
                # `const struct bt_mesh_comp[]`.
                exp_tags = [
                    ['DW_TAG_const_type', 'DW_TAG_structure_type'],
                    ['DW_TAG_const_type', 'DW_TAG_array_type', 'DW_TAG_const_type', 'DW_TAG_structure_type'],
                ]
                type_die = die
                max_length = max(len(arr) for arr in exp_tags)
                try:
                    for i in range(max_length):
                        type_die_type = type_die.attributes.get('DW_AT_type')
                        if type_die_type is None:
                            raise Exception('DW_AT_type is missing')
                        type_die = type_die.get_DIE_from_attribute('DW_AT_type')
                        for tags in exp_tags:
                            if len(tags) > i and type_die.tag == tags[i]:
                                break
                        else:
                            raise Exception('Wrong DW_AT_type')
                        name = type_die.attributes.get('DW_AT_name')
                        if name and name.value == b'bt_mesh_comp':
                            break
                    else:
                        continue
                except Exception:
                    continue

                comp_data_arr.append(addr)

        if comp_data_arr is None or len(comp_data_arr) == 0:
            raise Exception("Could not find composition data in .elf file")

        return comp_data_arr


def read_comp_data(elf_path, addr, kconfigs):
    """
    Reads content of the composition data variable from .elf file.

    Parameters:
        elf_path (ELFFile): ELFFile instance
        addr (int): Address of the composition data variable
        kconfigs (KConfig): A KConfig object representing Kconfig options used for the firmware to compile with
    Returns:
        Parsed Composition data
    """

    label_cnt = int(kconfigs['CONFIG_BT_MESH_LABEL_COUNT']) if 'CONFIG_BT_MESH_LABEL_COUNT' in kconfigs.keys() else 0
    lcd_srv = (kconfigs['CONFIG_BT_MESH_LARGE_COMP_DATA_SRV'] == 'y') if 'CONFIG_BT_MESH_LARGE_COMP_DATA_SRV' in kconfigs.keys() else False

    with open(elf_path, 'rb') as elf_file:
        elf = ELFFile(elf_file)
        cdp0_value = read_symbol_data(elf, addr)

        # The format of the composition data is defined by `struct bt_mesh_comp` type.
        # The format of an element is defined by `struct bt_mesh_elem` type.
        # The format of a model is defined by `struct bt_mesh_model` type.
        # All types are declared in `zephyr/include/zephyr/bluetooth/mesh/access.h`.
        for comp_data_entry in struct.iter_unpack('HHHHII', cdp0_value):
            cid, pid, vid, _, elems_count, elems_ptr = comp_data_entry

            comp = Comp0(cid, pid, vid, kconfigs)

            elems_value = read_symbol_data(elf, elems_ptr)
            elems_iter = struct.iter_unpack('IHBBII', elems_value)
            i = 0

            for elem in elems_iter:
                i += 1
                if i > elems_count:
                    raise Exception('Extracted more elems than \'elem_count\'')
                __rt, loc, sig_count, vnd_count, sig_ptr, vnd_ptr = elem
                elem_item = comp.elem_add(loc)

                def models_unpack(ptr, elem_item, vnd):
                    models_value = read_symbol_data(elf, ptr)
                    model_format = 'HHIIIHHIHH' + ('I' if label_cnt > 0  else '') + 'II' + ('I' if lcd_srv else '')
                    models_iter = struct.iter_unpack(model_format, models_value)

                    for model in models_iter:
                        id1, id2, __rt, __pub, __keys, __keys_cnt, _, __groups, __groups_cnt, _, __uuids, __op, __cb = model
                        if not vnd:
                            elem_item.sig_model_add(id1)
                        else:
                            elem_item.vnd_model_add(id1, id2)

                if sig_count > 0:
                    models_unpack(sig_ptr, elem_item, False)
                if vnd_count > 0:
                    models_unpack(vnd_ptr, elem_item, True)
            yield comp

def parse_comp_data(elf_path, kconfigs):
    """
    Extract composition data from .elf and .config file.

    Parameters:
        kconfigs (KConfig): A KCoonfig object representing Kconfig options used for the firmware to compile with
    Returns:
        List of parsed composition data
    """
    try:
        addrs = find_comp_data_from_dwarf(elf_path)
        return [comp
                for addr in addrs
                for comp in read_comp_data(elf_path, addr, kconfigs)]
    except Exception as err:
        raise Exception("Failed to extract composition data from .elf file") from err

def encoded_metadata_get(version, comp, binary_size, core_type):
    elem_cnt = len(comp.elems)

    bytestring = bytearray()
    bytestring.append(version["major"])
    bytestring.append(version["minor"])
    bytestring.extend(version["revision"].to_bytes(2, 'little'))
    bytestring.extend(version["build_number"].to_bytes(4, 'little'))
    bytestring.extend(binary_size.to_bytes(3, 'little'))
    bytestring.append(core_type)
    bytestring.extend(comp.hash_generate().to_bytes(4, 'little'))
    bytestring.extend(elem_cnt.to_bytes(2, 'little'))
    return bytestring

def input_parse():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument('--bin-path', required=True, type=str)
    parser.add_argument('--print-metadata', action='store_true')
    return parser.parse_known_args()[0]

def existing_metadata_print(path):
    try:
        metadata_file = open(path, 'r')
        print(json.dumps(json.load(metadata_file), indent=4))
    except Exception as err :
        raise Exception("Failed to get existing metadata") from err

if __name__ == "__main__":
    try:
        args = input_parse()

        sysbuild_config_path = os.path.abspath(os.path.join(args.bin_path, '.config.sysbuild'))

        if os.path.isfile(sysbuild_config_path):
            # Sysbuild
            zip_path = os.path.abspath(os.path.join(args.bin_path, '..', '..', 'dfu_application.zip'))
            sysbuild = True
        else:
            # Child/parent image
            zip_path = os.path.abspath(os.path.join(args.bin_path, 'dfu_application.zip'))
            sysbuild = False

        metadata_path = os.path.abspath(os.path.join(args.bin_path, FILE_NAME))
        config_path = os.path.abspath(os.path.join(args.bin_path, '.config'))
        kconfigs = KConfig.from_file(config_path)
        kernel_name = kconfigs['CONFIG_KERNEL_BIN_NAME'].replace("\"", "")
        elf_path = os.path.abspath(os.path.join(args.bin_path, (kernel_name + '.elf')))

        if args.print_metadata:
            # Caller requests already generated metadata
            existing_metadata_print(metadata_path)
            sys.exit(0)

        zip = ZipFile(zip_path, "a")
        if FILE_NAME_IN_ZIP in zip.namelist():
            # Mesh metadata already present in zip file
            sys.exit(0)

        comps = parse_comp_data(elf_path, kconfigs)
        version = kconfigs.version_parse()

        if sysbuild:
            binary_size = os.path.getsize(os.path.join(args.bin_path, (kernel_name + '.signed.bin')))
        else:
            binary_size = os.path.getsize(os.path.join(args.bin_path, 'app_update.bin'))
        core_type = 1
        json_data = []

        for comp in comps:
            encoded_metadata = encoded_metadata_get(version, comp, binary_size, core_type)
            json_data.append({
                "sign_version": version,
                "binary_size": binary_size,
                "core_type": core_type,
                "composition_data": comp.dict_generate(),
                "composition_hash": str(hex(comp.hash_generate())),
                "encoded_metadata": str(encoded_metadata.hex()),
            })

        with open(metadata_path, "w") as outfile:
            outfile.write(json.dumps(json_data if len(json_data) > 1 else json_data[0], indent=4))
        zip.write(metadata_path, FILE_NAME_IN_ZIP)
        zip.close()

        print("Bluetooth Mesh Composition metadata generated:")
        if len(json_data) > 1:
            print(f"\t{len(json_data)} composition data instances found.")
            print(f"\tAll metadata variants written to: {zip_path}")
        else:
            print(f"\tEncoded metadata: {json_data[0]['encoded_metadata']}")
            print(f"\tFull metadata written to: {zip_path}")
    except Exception:
        exit_with_error_msg()
