#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""
Script to build and program the nRF5340 Audio project to multiple devices
"""

import argparse
import sys
import shutil
import os
import json
import subprocess
import re
from colorama import Fore, Style
from prettytable import PrettyTable
from nrf5340_audio_dk_devices import DeviceConf, BuildConf, SelectFlags
from program import program_threads_run

TARGET_BOARD_NRF5340_AUDIO_DK_APP_NAME = 'nrf5340_audio_dk_nrf5340_cpuapp'
TARGET_BOARD_NRF5340_AUDIO_DK_NET_NAME = 'nrf5340_audio_dk_nrf5340_cpunet'

TARGET_CORE_APP_FOLDER = '../..'
TARGET_CORE_NET_FOLDER = '../../bin'
TARGET_DEV_HEADSET_FOLDER = 'build/dev_headset'
TARGET_DEV_GATEWAY_FOLDER = 'build/dev_gateway'
TARGET_DEV_BLE_FOLDER = 'dev_ble'
TARGET_RELEASE_FOLDER = 'build_release'
TARGET_DEBUG_FOLDER = 'build_debug'

COMP_FLAG_DEV_HEADSET = ' -DOVERLAY_CONFIG="overlay-headset.conf '
COMP_FLAG_DEV_GATEWAY = ' -DOVERLAY_CONFIG="overlay-gateway.conf '
COMP_FLAG_RELEASE = ' overlay-release.conf"'
COMP_FLAG_DEBUG = ' overlay-debug.conf"'

PRISTINE_FLAG = ' --pristine'


def __print_add_color(status):
    if status == SelectFlags.FAIL:
        return Fore.RED + status + Style.RESET_ALL
    elif status == SelectFlags.DONE:
        return Fore.GREEN + status + Style.RESET_ALL
    return status


def __print_dev_conf(device_list):
    """Print settings in a formatted manner"""
    table = PrettyTable()
    table.field_names = ["snr", "snr conn", "device", "only reboot",
                         "core app programmed", "core net programmed"]
    for device in device_list:
        row = []
        row.append(device.nrf5340_audio_dk_snr)
        if device.nrf5340_audio_dk_snr_connected:
            row.append(Fore.GREEN + str(device.nrf5340_audio_dk_snr_connected)
                       + Style.RESET_ALL)
        else:
            row.append(Fore.YELLOW + str(device.nrf5340_audio_dk_snr_connected)
                       + Style.RESET_ALL)

        row.append(device.nrf5340_audio_dk_device)
        row.append(__print_add_color(device.only_reboot))
        row.append(__print_add_color(device.core_app_programmed))
        row.append(__print_add_color(device.core_net_programmed))

        table.add_row(row)
    print(table)


def __build_cmd_get(core, device, build, pristine):
    build_cmd = "west build "
    dest_folder = ""
    compiler_flags = ""

    if core == "app":
        build_cmd += (TARGET_CORE_APP_FOLDER + " -b " +
                      TARGET_BOARD_NRF5340_AUDIO_DK_APP_NAME)
        dest_folder += TARGET_CORE_APP_FOLDER
        if device == "headset":
            compiler_flags += COMP_FLAG_DEV_HEADSET
            dest_folder += "/" + TARGET_DEV_HEADSET_FOLDER
        elif device == "gateway":
            compiler_flags += COMP_FLAG_DEV_GATEWAY
            dest_folder += "/" + TARGET_DEV_GATEWAY_FOLDER

    if core == "net":
        # The net core is precompiled
        dest_folder += TARGET_CORE_NET_FOLDER
        build_cmd = ""
        compiler_flags = ""
        return build_cmd, dest_folder, compiler_flags

    if build == "debug":
        compiler_flags += " " + COMP_FLAG_DEBUG
        dest_folder += "/" + TARGET_DEBUG_FOLDER
    elif build == "release":
        compiler_flags += " " + COMP_FLAG_RELEASE
        dest_folder += "/" + TARGET_RELEASE_FOLDER

    if pristine:
        build_cmd += " -p "

    return build_cmd, dest_folder, compiler_flags


def __build_module(build_config):
    build_cmd, dest_folder, compiler_flags = \
        __build_cmd_get(build_config.core, build_config.device,
                        build_config.build, build_config.pristine)
    west_str = build_cmd + " -d " + dest_folder
    curr_folder = os.getcwd()

    if build_config.pristine and os.path.exists(curr_folder + "/" +
                                                dest_folder):
        shutil.rmtree(dest_folder)

    # Only add compiler flags if folder doesn't exist already
    if not os.path.exists(curr_folder + "/" + dest_folder):
        west_str += compiler_flags

    print("Run: " + west_str)

    ret_val = os.system(west_str)

    if ret_val:
        raise Exception("cmake error: " + str(ret_val))


def __find_snr():
    """Rebooting or programming requires connected programmer/debugger"""

    # Use nrfjprog executable for WSL compatibility
    stdout = subprocess.check_output('nrfjprog --ids',
                                     shell=True).decode('utf-8')
    snrs = re.findall(r'([\d]+)', stdout)

    if not snrs:
        print("No programmer/debugger connected to PC")

    return list(map(int, snrs))


def __populate_hex_paths(dev, options):
    """Poplulate hex paths where relevant"""
    if dev.core_net_programmed == SelectFlags.TBD:
        _, dest_folder, _ = __build_cmd_get("net", None, options.build,
                                            options.pristine)

        hex_files_found = []
        for file in os.listdir(dest_folder):
            if file.endswith(".hex"):
                hex_files_found.append(file)

        if len(hex_files_found) == 0:
            raise Exception("Found no net core hex file in folder: " +
                            dest_folder)
        elif len(hex_files_found) > 1:
            raise Exception("Found more than one hex file in folder: " +
                            dest_folder)
        else:
            dev.hex_path_net = dest_folder + "/" + hex_files_found[0]
            print("Using NET hex: " + dev.hex_path_net)

    if dev.core_app_programmed == SelectFlags.TBD:
        _, dest_folder, _ = __build_cmd_get("app", dev.nrf5340_audio_dk_device,
                                            options.build, options.pristine)
        dev.hex_path_app = dest_folder + "/zephyr/zephyr.hex"


def __finish(device_list):
    """Finish script. Print report"""
    print("build_prog.py finished. Report:")
    __print_dev_conf(device_list)
    exit(0)


def __option_match_device(option_dev, hw_dev):
    """Check if the selected option matches the given device """
    if option_dev == "both":
        return True
    elif option_dev == hw_dev:
        return True
    return False


def __main():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description="This script builds and programs the nRF5340 Audio \
                     project on Windows and Linux")
    parser.add_argument("-r", "--only_reboot", default=False,
                        action='store_true',
                        help="Only reboot, no building or programming")
    parser.add_argument("-p", "--program", default=False,
                        action='store_true',
                        help="Will program and reboot nRF5340 Audio DK.")
    parser.add_argument("-c", "--core",
                        choices=["app", "net", "both"],
                        help="app, net, or both")
    parser.add_argument("--pristine", default=False,
                        action='store_true',
                        help="Will build cleanly")
    parser.add_argument("-b", "--build", required="-p" in sys.argv or
                        "--program" in sys.argv,
                        choices=["release", "debug"], help="Release or debug")
    parser.add_argument("-d", "--device", required=("-b" in sys.argv or
                                                    "--build" in sys.argv)
                        and ("both" in sys.argv or "app" in sys.argv),
                        choices=["headset", "gateway", "both"],
                        help="nRF5340 Audio on the application core can be \
                        built for either ordinary headset \
                        (earbuds/headphone..) use or gateway (USB dongle)")

    parser.add_argument("-s", "--sequential",
                        required=False,
                        dest="sequential_prog",
                        type=bool,
                        default=False,
                        nargs="?",
                        const=True,
                        help="Run nrfjprog sequentially instead of in \
                        parallel.")
    options = parser.parse_args(args=sys.argv[1:])

    boards_snr_connected = __find_snr()

    # Update device list
    # This JSON file should be altered by the developer.
    # Then run git update-index --skip-worktree FILENAME to avoid changes
    # being pushed
    dev_file = open('nrf5340_audio_dk_devices.json')
    dev_arr = json.load(dev_file)
    device_list = []
    for dev in dev_arr:
        device_list.append(DeviceConf(dev["nrf5340_audio_dk_snr"],
                                      dev["nrf5340_audio_dk_dev"],
                                      dev["channel"]))

    # Match connected SNRs to list
    if boards_snr_connected:
        for dev in device_list:
            for snr in boards_snr_connected:
                if dev.nrf5340_audio_dk_snr == snr:
                    dev.nrf5340_audio_dk_snr_connected = True

            if options.only_reboot:
                dev.only_reboot = SelectFlags.TBD
                continue

            if options.core == "app" or options.core == "both":
                if __option_match_device(options.device,
                                         dev.nrf5340_audio_dk_device):
                    dev.core_app_programmed = SelectFlags.TBD

            if options.core == "net" or options.core == "both":
                if __option_match_device(options.device,
                                         dev.nrf5340_audio_dk_device):
                    dev.core_net_programmed = SelectFlags.TBD

    else:
        print("No snrs connected")

    __print_dev_conf(device_list)

    # Initialization step finsihed
    # Reboot step start

    if options.only_reboot:
        program_threads_run(device_list, sequential=options.sequential_prog)
        __finish(device_list)

    # Reboot step finished
    # Build step start

    if options.build:
        print("Invoking build step")
        build_configs = []
        if options.core == "both" or options.core == "app":
            if options.device == "both":
                build_configs.append(BuildConf("app", "headset",
                                               options.pristine,
                                               options.build))
                build_configs.append(BuildConf("app", "gateway",
                                               options.pristine,
                                               options.build))
            elif options.device == "headset":
                build_configs.append(BuildConf("app", "headset",
                                               options.pristine,
                                               options.build))
            elif options.device == "gateway":
                build_configs.append(BuildConf("app", "gateway",
                                               options.pristine,
                                               options.build))
        if options.core == "both" or options.core == "net":
            print("Net core uses precompiled hex")

        for build_cfg in build_configs:
            __build_module(build_cfg)

    # Build step finished
    # Program step start

    if options.program:
        for dev in device_list:
            __populate_hex_paths(dev, options)

    program_threads_run(device_list, sequential=options.sequential_prog)

    # Program step finished

    __finish(device_list)


if __name__ == "__main__":
    __main()
