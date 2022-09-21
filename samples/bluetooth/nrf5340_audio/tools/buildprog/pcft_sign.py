#!/usr/bin/env python3
#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
Sign PCFT BT Controller with B0N
"""

import argparse
import sys
import shutil
import os
from pathlib import Path

PCFT_HEX = "new_fw_info_pcft.hex"
FINAL_PCFT_HEX = "pcft_CPUNET.hex"
FINAL_PCFT_UPDATE_BIN = "pcft_net_core_update.bin"
ZEPHYR_BASE = os.environ["ZEPHYR_BASE"]
MANUALLY_SIGN_DIR = Path(__file__).resolve().parent
BIN_DIR = (MANUALLY_SIGN_DIR / "../../bin").resolve()


def awklike(field_str, filename):
    """ A function like unix awk to split string"""
    ret = ''
    try:
        with open(filename, encoding="utf8") as file_pointer:
            for line in file_pointer:
                if field_str in line:
                    ret = line.replace(field_str, '').replace(
                        '\n', '').replace('"', '')
                    break
    except (OSError, IOError) as exp:
        print(exp)
    return ret


def sign(orig_pcft_hex, build_dir):
    """ A function to combine and sign PCFT"""

    if os.name == 'nt':
        folder_slash ='\\'
    else:
        folder_slash ='/'

    # Make sure we input build_dir as absolute path
    indices = [i for i, c in enumerate(build_dir) if c == folder_slash]
    final_file_prefix = build_dir[indices[-2]+1:].replace(folder_slash, '_')+'_'

    # RETEIVE setting value from .config
    # "${ZEPHYR_BASE}/../bootloader/mcuboot/root-rsa-2048.pem"

    mcuboot_rsa_key = awklike('CONFIG_BOOT_SIGNATURE_KEY_FILE=', build_dir +
                              '/mcuboot/zephyr/.config')

    #'\\' is used for Windows, '/' is used for other Operating Systems like Linux.
    if ('/' in mcuboot_rsa_key) or ('\\' in  mcuboot_rsa_key):
        print('absolute path')
        # Zephyr script convert folder separator to '/'. Should do the same here no matter Windows or not.
        if folder_slash in mcuboot_rsa_key:
            mcuboot_rsa_key.replace(folder_slash, '/')
    else:
        print('relative path')
        mcuboot_rsa_key = ZEPHYR_BASE + '/../bootloader/mcuboot/' + mcuboot_rsa_key

    # IMAGE_VERSION="0.0.0+1"
    image_version = awklike('CONFIG_MCUBOOT_IMAGE_VERSION=',
                            build_dir + '/zephyr/.config')
    # VALIDATION_MAGIC_VALUE="0x281ee6de,0x86518483,79106"
    fw_info_magic_common = awklike('CONFIG_FW_INFO_MAGIC_COMMON=', build_dir +
                                   '/hci_rpmsg/zephyr/.config')
    fw_info_magic_firmware_info = awklike('CONFIG_FW_INFO_MAGIC_FIRMWARE_INFO=', build_dir +
                                          '/hci_rpmsg/zephyr/.config')
    sb_validation_info_magic = awklike('CONFIG_SB_VALIDATION_INFO_MAGIC=', build_dir +
                                       '/hci_rpmsg/zephyr/.config')
    fw_info_firmware_version = awklike('CONFIG_FW_INFO_FIRMWARE_VERSION=', build_dir +
                                       '/hci_rpmsg/zephyr/.config')
    fw_info_valid_val = awklike('CONFIG_FW_INFO_VALID_VAL=', build_dir +
                                '/hci_rpmsg/zephyr/.config')
    temp_ver = int(awklike('CONFIG_SB_VALIDATION_INFO_VERSION=', build_dir +
                           '/hci_rpmsg/zephyr/.config'))
    temp_hwid = int(awklike('CONFIG_FW_INFO_HARDWARE_ID=', build_dir +
                            '/hci_rpmsg/zephyr/.config'))
    temp_valid_crypto_id = int(awklike('CONFIG_SB_VALIDATION_INFO_CRYPTO_ID=', build_dir +
                                       '/hci_rpmsg/zephyr/.config'))
    temp_crypto_id = int(awklike('CONFIG_FW_INFO_CRYPTO_ID=', build_dir +
                                 '/hci_rpmsg/zephyr/.config'))
    temp_compat_id = int(awklike('CONFIG_FW_INFO_MAGIC_COMPATIBILITY_ID=', build_dir +
                                 '/hci_rpmsg/zephyr/.config'))
    magic_compatibility_validation_info = temp_ver + (temp_hwid << 8) + (temp_valid_crypto_id << 16)\
        + (temp_compat_id << 24)
    magic_compatibility_info = temp_ver + (temp_hwid << 8) + (temp_crypto_id << 16)\
        + (temp_compat_id << 24)

    validation_magic_value = f'{fw_info_magic_common},{sb_validation_info_magic},\
{magic_compatibility_validation_info}'

    firmware_info_magic = f'{fw_info_magic_common},{fw_info_magic_firmware_info},\
{magic_compatibility_info}'

    # 0x01008800
    pm_net_app_address = int(awklike('PM_APP_ADDRESS=', build_dir +
                                     '/hci_rpmsg/pm_CPUNET.config'), 16)
    # 0xc200/0x10200
    #PM_APP_APP_ADDRESS = int(awklike('PM_APP_ADDRESS=', build_dir + '/pm.config'), 16)
    pm_app_app_address = awklike('PM_APP_ADDRESS=', build_dir + '/pm.config')
    # CONFIG_FW_INFO_OFFSET 0x200
    fw_info_offset = int(awklike('CONFIG_FW_INFO_OFFSET=', build_dir +
                                 '/hci_rpmsg/b0n/zephyr/.config'), 16)
    # nRF5340_CPUAPP_QKAA
    soc_name = awklike('CONFIG_SOC=', build_dir + '/zephyr/.config')
    # nRF5340_CPUNET_QKAA
    net_soc_name = awklike('CONFIG_SOC=', build_dir +
                           '/hci_rpmsg/zephyr/.config')
    # CONFIG_DOMAIN_CPUNET_BOARD net_core_app_update.binboard
    net_core_app_binboard = awklike(
        'CONFIG_DOMAIN_CPUNET_BOARD=', build_dir + '/zephyr/.config')
    # CONFIG_BOARD="nrf5340_audio_nrf5340_cpuapp"
    config_board = awklike('CONFIG_BOARD=', build_dir + '/zephyr/.config')

    net_core_fw_info_address = (pm_net_app_address + fw_info_offset)
    net_core_fw_info_address = f"0x{net_core_fw_info_address:08X}"

    # Inject FW_INFO from .config
    os_cmd = f'python fw_info_data.py  --input {orig_pcft_hex} --output-hex {build_dir}/{PCFT_HEX}\
    --offset {net_core_fw_info_address} --magic-value {firmware_info_magic}\
    --fw-version {fw_info_firmware_version} --fw-valid-val {fw_info_valid_val}'

    ret_val = os.system(os_cmd)
    if ret_val:
        raise Exception("python error: " + str(ret_val))

    # 240
    num_ver_counter_slots = int(awklike('CONFIG_SB_NUM_VER_COUNTER_SLOTS=', build_dir +
                                        '/hci_rpmsg/zephyr/.config'), 16)
    pm_partition_size_provision = int(awklike('CONFIG_PM_PARTITION_SIZE_PROVISION=', build_dir +
                                              '/hci_rpmsg/zephyr/.config'), 16)
    cpunet_pm_app_address = hex(int(awklike('PM_APP_ADDRESS=', build_dir +
                                            '/hci_rpmsg/pm_CPUNET.config'), 16))
    pm_provision_address = int(awklike('PM_PROVISION_ADDRESS=', build_dir +
                                       '/hci_rpmsg/pm_CPUNET.config'), 16)
    pm_mcuboot_secondary_size = int(awklike('PM_MCUBOOT_SECONDARY_SIZE=', build_dir +
                                            '/pm.config'), 16)

    os_cmd = f'python {ZEPHYR_BASE}/../nrf/scripts/bootloader/hash.py --in {build_dir}/{PCFT_HEX}\
    > {build_dir}/app_firmware.sha256'

    ret_val = os.system(os_cmd)
    if ret_val:
        raise Exception("python error: " + str(ret_val))

    os_cmd = f'python {ZEPHYR_BASE}/../nrf/scripts/bootloader/do_sign.py --private-key\
    {build_dir}/hci_rpmsg/zephyr/GENERATED_NON_SECURE_SIGN_KEY_PRIVATE.pem --in\
    {build_dir}/app_firmware.sha256 > {build_dir}/app_firmware.signature'

    ret_val = os.system(os_cmd)
    if ret_val:
        raise Exception("python error: " + str(ret_val))

    os_cmd = f'python {ZEPHYR_BASE}/../nrf/scripts/bootloader/validation_data.py\
    --input {build_dir}/{PCFT_HEX} --output-hex {build_dir}/signed_by_b0_pcft.hex\
    --output-bin {build_dir}/signed_by_b0_pcft.bin --offset 0 --signature\
    {build_dir}/app_firmware.signature --public-key\
    {build_dir}/hci_rpmsg/zephyr/nrf/subsys/bootloader/generated/public.pem\
    --magic-value {validation_magic_value}'

    ret_val = os.system(os_cmd)
    if ret_val:
        raise Exception("python error: " + str(ret_val))

    # Generate net_core_app_update.bin
    os_cmd = f'python {ZEPHYR_BASE}/../bootloader/mcuboot/scripts/imgtool.py sign --key\
    {mcuboot_rsa_key} --header-size {fw_info_offset} --align 4 --version {image_version}\
    --pad-header --slot-size {pm_mcuboot_secondary_size}  {build_dir}/signed_by_b0_pcft.bin\
    {build_dir}/{FINAL_PCFT_UPDATE_BIN}'

    ret_val = os.system(os_cmd)
    if ret_val:
        raise Exception("python error: " + str(ret_val))

    os_cmd = f'python {ZEPHYR_BASE}/../nrf/scripts/bootloader/provision.py\
    --s0-addr {cpunet_pm_app_address} --provision-addr {pm_provision_address}\
    --public-key-files {build_dir}/hci_rpmsg/zephyr/nrf/subsys/bootloader/generated/public.pem,\
{build_dir}/hci_rpmsg/zephyr/GENERATED_NON_SECURE_PUBLIC_0.pem,\
{build_dir}/hci_rpmsg/zephyr/GENERATED_NON_SECURE_PUBLIC_1.pem\
    --output {build_dir}/provision.hex --num-counter-slots-version {num_ver_counter_slots}\
    --max-size {pm_partition_size_provision}'

    ret_val = os.system(os_cmd)
    if ret_val:
        raise Exception("python error: " + str(ret_val))

    os_cmd = f'python {ZEPHYR_BASE}/scripts/build/mergehex.py -o {build_dir}/b0n_container.hex\
    {build_dir}/{PCFT_HEX} {build_dir}/provision.hex'

    ret_val = os.system(os_cmd)
    if ret_val:
        raise Exception("python error: " + str(ret_val))

    os_cmd = f'python {ZEPHYR_BASE}/scripts/build/mergehex.py -o {build_dir}/{FINAL_PCFT_HEX}\
    --overlap=replace {build_dir}/hci_rpmsg/b0n/zephyr/zephyr.hex  {build_dir}/b0n_container.hex\
    {build_dir}/provision.hex {build_dir}/{PCFT_HEX} {build_dir}/signed_by_b0_pcft.hex'

    ret_val = os.system(os_cmd)
    if ret_val:
        raise Exception("python error: " + str(ret_val))

    # Replace built net_core
    src_path = f'{build_dir}/{FINAL_PCFT_UPDATE_BIN}'
    dst_path = f'{build_dir}/zephyr/net_core_app_update.bin'
    os.remove(dst_path)
    shutil.copy(src_path, dst_path)

    src_path = f'{build_dir}/{FINAL_PCFT_HEX}'
    dst_path = f'{build_dir}/zephyr/net_core_app_signed.hex'
    os.remove(dst_path)
    shutil.copy(src_path, dst_path)

    # cp $build_dir/${FINAL_PCFT_HEX} ${BASEDIR}/../../bin
    src_path = f'{build_dir}/{FINAL_PCFT_HEX}'
    dst_path = f'{BIN_DIR}/{final_file_prefix}{FINAL_PCFT_HEX}'
    shutil.copy(src_path, dst_path)

    # Generate merged_domains.hex
    os_cmd = f'python {ZEPHYR_BASE}/scripts/build/mergehex.py -o {build_dir}/zephyr/merged_domains.hex\
    {build_dir}/{FINAL_PCFT_HEX} {build_dir}/zephyr/merged.hex'

    ret_val = os.system(os_cmd)
    # Generate dfu_application.zip
    # set(generate_script_params
    # "${app_core_binary_name}load_address=$<TARGET_PROPERTY:partition_manager,PM_APP_ADDRESS>"
    # "${app_core_binary_name}image_index=0"
    # "${app_core_binary_name}slot_index_primary=1"
    # "${app_core_binary_name}slot_index_secondary=2"
    # "${app_core_binary_name}version_MCUBOOT=${CONFIG_MCUBOOT_IMAGE_VERSION}"
    # "${net_core_binary_name}image_index=1"
    # "${net_core_binary_name}slot_index_primary=3"
    # "${net_core_binary_name}slot_index_secondary=4"
    # "${net_core_binary_name}load_address=$<TARGET_PROPERTY:partition_manager,\
    # CPUNET_PM_APP_ADDRESS>"
    # "${net_core_binary_name}board=${CONFIG_DOMAIN_CPUNET_BOARD}"
    # "${net_core_binary_name}version=${net_core_version}"
    # "${net_core_binary_name}soc=${net_core_soc}"
    # )
    os.remove(f'{build_dir}/zephyr/dfu_application.zip')
    os_cmd = f'python {ZEPHYR_BASE}/../nrf/scripts/bootloader/generate_zip.py --bin-files\
    {build_dir}/zephyr/app_update.bin {build_dir}/zephyr/net_core_app_update.bin\
    --output {build_dir}/zephyr/dfu_application.zip --name nrf5340_audio\
    --meta-info-file {build_dir}/zephyr/zephyr.meta\
    app_update.binload_address={pm_app_app_address}\
    app_update.binimage_index=0\
    app_update.binslot_index_primary=1\
    app_update.binslot_index_secondary=2\
    app_update.binversion_MCUBOOT={image_version}\
    net_core_app_update.binimage_index=1\
    net_core_app_update.binslot_index_primary=3\
    net_core_app_update.binslot_index_secondary=4\
    net_core_app_update.binload_address={cpunet_pm_app_address}\
    net_core_app_update.binboard={net_core_app_binboard}\
    net_core_app_update.binversion={fw_info_firmware_version}\
    net_core_app_update.binsoc={net_soc_name}\
    type=application board={config_board} soc={soc_name}'

    ret_val = os.system(os_cmd)

    # For DFU stress test copy product to bin folder
    # Copy dfu_applications.zip
    shutil.copy(f'{build_dir}/zephyr/dfu_application.zip',
                f'{BIN_DIR}/{final_file_prefix}dfu_application_{image_version}.zip')

    # Copy app bin
    shutil.copy(f'{build_dir}/zephyr/app_update.bin',
                f'{BIN_DIR}/{final_file_prefix}app_update_{image_version}.bin')

    # Copy net bin
    shutil.copy(f'{build_dir}/zephyr/net_core_app_update.bin'.format(),
                f'{BIN_DIR}/{final_file_prefix}net_core_app_update_{fw_info_firmware_version}.bin')


def __main():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description="This script sign and generate netcore hex and upgradable binary for the\
         nRF5340 Audio project on Windows and Linux")
    parser.add_argument('-i', '--input', required=True, type=str,
                        help='Input hex file.')
    parser.add_argument("-b", "--build_dir", required=True, type=str,
                        help='Build folder.')
    options = parser.parse_args(args=sys.argv[1:])
    sign(options.input, options.build_dir)


if __name__ == "__main__":
    __main()
