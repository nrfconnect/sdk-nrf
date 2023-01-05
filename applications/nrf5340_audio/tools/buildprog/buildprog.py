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
from nrf5340_audio_dk_devices import (
    BuildType,
    Channel,
    DeviceConf,
    BuildConf,
    AudioDevice,
    SelectFlags,
    Core,
)
from program import program_threads_run
from pathlib import Path


BUILDPROG_FOLDER = Path(__file__).resolve().parent
NRF5340_AUDIO_FOLDER = (BUILDPROG_FOLDER / "../..").resolve()
USER_CONFIG = BUILDPROG_FOLDER / "nrf5340_audio_dk_devices.json"

TARGET_BOARD_NRF5340_AUDIO_DK_APP_NAME = "nrf5340_audio_dk_nrf5340_cpuapp"

TARGET_CORE_APP_FOLDER = NRF5340_AUDIO_FOLDER
TARGET_CORE_NET_FOLDER = NRF5340_AUDIO_FOLDER / "bin"
TARGET_DEV_HEADSET_FOLDER = NRF5340_AUDIO_FOLDER / "build/dev_headset"
TARGET_DEV_GATEWAY_FOLDER = NRF5340_AUDIO_FOLDER / "build/dev_gateway"

TARGET_RELEASE_FOLDER = "build_release"
TARGET_DEBUG_FOLDER = "build_debug"


def __print_add_color(status):
    if status == SelectFlags.FAIL:
        return Fore.RED + status.value + Style.RESET_ALL
    elif status == SelectFlags.DONE:
        return Fore.GREEN + status.value + Style.RESET_ALL
    return status.value


def __print_dev_conf(device_list):
    """Print settings in a formatted manner"""
    table = PrettyTable()
    table.field_names = [
        "snr",
        "snr conn",
        "device",
        "only reboot",
        "core app programmed",
        "core net programmed",
    ]
    for device in device_list:
        row = []
        row.append(device.nrf5340_audio_dk_snr)
        color = Fore.GREEN if device.snr_connected else Fore.YELLOW
        row.append(color + str(device.snr_connected) + Style.RESET_ALL)
        row.append(device.nrf5340_audio_dk_dev.value)
        row.append(__print_add_color(device.only_reboot))
        row.append(__print_add_color(device.core_app_programmed))
        row.append(__print_add_color(device.core_net_programmed))

        table.add_row(row)
    print(table)


def __build_cmd_get(core: Core, device: AudioDevice, build: BuildType, pristine, options):
    if core == Core.app:
        build_cmd = f"west build {TARGET_CORE_APP_FOLDER} -b {TARGET_BOARD_NRF5340_AUDIO_DK_APP_NAME}"
        if device == AudioDevice.headset:
            device_flag = "-DCONFIG_AUDIO_DEV=1"
            dest_folder = TARGET_DEV_HEADSET_FOLDER
        elif device == AudioDevice.gateway:
            device_flag = "-DCONFIG_AUDIO_DEV=2"
            dest_folder = TARGET_DEV_GATEWAY_FOLDER
        else:
            raise Exception("Invalid device!")
        if build == BuildType.debug:
            release_flag = ""
            dest_folder /= TARGET_DEBUG_FOLDER
        elif build == BuildType.release:
            release_flag = " -DCONF_FILE=prj_release.conf"
            dest_folder /= TARGET_RELEASE_FOLDER
        else:
            raise Exception("Invalid build type!")

        if options.mcuboot == 'internal':
            device_flag += " -DCONFIG_AUDIO_DFU=1"
        elif options.mcuboot == 'external':
            device_flag += " -DCONFIG_AUDIO_DFU=2"
        if options.min_b0n:
            device_flag += " -DCONFIG_B0N_MINIMAL=y"

        if options.nrf21540:
            device_flag += " -DSHIELD=nrf21540_ek_fwd"

        if os.name == 'nt':
            release_flag = release_flag.replace('\\', '/')

        if pristine:
            build_cmd += " -p"

    elif core == Core.net:
        # The net core is precompiled
        dest_folder = TARGET_CORE_NET_FOLDER
        build_cmd = ""
        device_flag = ""
        release_flag = ""

    return build_cmd, dest_folder, device_flag, release_flag


def __build_module(build_config, options):
    build_cmd, dest_folder, device_flag, release_flag = __build_cmd_get(
        build_config.core,
        build_config.device,
        build_config.build,
        build_config.pristine,
        options,
    )
    west_str = f"{build_cmd} -d {dest_folder} "

    if build_config.pristine and dest_folder.exists():
        shutil.rmtree(dest_folder)

    # Only add compiler flags if folder doesn't exist already
    if not dest_folder.exists():
        west_str = west_str + device_flag + release_flag

    print("Run: " + west_str)

    ret_val = os.system(west_str)

    if ret_val:
        raise Exception("cmake error: " + str(ret_val))


def __find_snr():
    """Rebooting or programming requires connected programmer/debugger"""

    # Use nrfjprog executable for WSL compatibility
    stdout = subprocess.check_output(
        "nrfjprog --ids", shell=True).decode("utf-8")
    snrs = re.findall(r"([\d]+)", stdout)

    if not snrs:
        print("No programmer/debugger connected to PC")

    return list(map(int, snrs))


def __populate_hex_paths(dev, options):
    """Poplulate hex paths where relevant"""

    _, temp_dest_folder, _, _ = __build_cmd_get(
        Core.app, dev.nrf5340_audio_dk_dev, options.build, options.pristine, options
    )

    dest_folder = temp_dest_folder
    if dev.core_app_programmed == SelectFlags.TBD:
        if options.mcuboot != '':
            dev.hex_path_app = dest_folder / "zephyr/merged.hex"
        else:
            dev.hex_path_app = dest_folder / "zephyr/zephyr.hex"

    if dev.core_net_programmed == SelectFlags.TBD:

        hex_files_found = [
            file for file in TARGET_CORE_NET_FOLDER.iterdir() if file.suffix == ".hex"
        ]

        if options.mcuboot != '':
            dev.hex_path_net = dest_folder / "zephyr/net_core_app_signed.hex"
        else:
            dest_folder = TARGET_CORE_NET_FOLDER

            if len(hex_files_found) == 0:
                raise Exception(
                    f"Found no net core hex file in folder: {dest_folder}")
            elif len(hex_files_found) > 1:
                raise Exception(
                    f"Found more than one hex file in folder: {dest_folder}")
            else:
                dev.hex_path_net = dest_folder / hex_files_found[0]
        print(f"Using NET hex: {dev.hex_path_net} for {dev}")


def __finish(device_list):
    """Finish script. Print report"""
    print("build_prog.py finished. Report:")
    __print_dev_conf(device_list)
    exit(0)


def __main():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=(
            "This script builds and programs the nRF5340 "
            "Audio project on Windows and Linux"
        ),
        allow_abbrev=False
    )
    parser.add_argument(
        "-r",
        "--only_reboot",
        default=False,
        action="store_true",
        help="Only reboot, no building or programming",
    )
    parser.add_argument(
        "-p",
        "--program",
        default=False,
        action="store_true",
        help="Will program and reboot nRF5340 Audio DK",
    )
    parser.add_argument(
        "-c",
        "--core",
        type=str,
        choices=[i.name for i in Core],
        help="Select which cores to include in build",
    )
    parser.add_argument(
        "--pristine", default=False, action="store_true", help="Will build cleanly"
    )
    parser.add_argument(
        "-b",
        "--build",
        required="-p" in sys.argv or "--program" in sys.argv,
        choices=[i.name for i in BuildType],
        help="Select the build type",
    )
    parser.add_argument(
        "-d",
        "--device",
        required=("-r" in sys.argv or "--only_reboot" in sys.argv)
        or (
            ("-b" in sys.argv or "--build" in sys.argv)
            and ("both" in sys.argv or "app" in sys.argv)
        ),
        choices=[i.name for i in AudioDevice],
        help=(
            "nRF5340 Audio on the application core can be "
            "built for either ordinary headset "
            "(earbuds/headphone..) use or gateway (USB dongle)"
        ),
    )
    parser.add_argument(
        "-s",
        "--sequential",
        action="store_true",
        dest="sequential_prog",
        default=False,
        help="Run nrfjprog sequentially instead of in \
                        parallel",
    )
    parser.add_argument(
        "-f",
        "--recover_on_fail",
        action="store_true",
        dest="recover_on_fail",
        default=False,
        help="Recover device if programming fails",
    )
    # DFU relative option
    parser.add_argument(
        "-M",
        "--min_b0n",
        dest="min_b0n",
        action='store_true',
        default=False,
        help="net core bootloader use minimal size build",
    )
    parser.add_argument(
        "-m",
        "--mcuboot",
        required=("-M" in sys.argv or "--min_b0n" in sys.argv),
        choices=["external", "internal"],
        default='',
        help="MCUBOOT with external, internal flash",
        )
    parser.add_argument(
        "--nrf21540",
        action="store_true",
        dest="nrf21540",
        default=False,
        help="Set when using nRF21540 for extra TX power",
        )
    options = parser.parse_args(args=sys.argv[1:])

    # Post processing for Enums
    if options.core is None:
        cores = []
    elif options.core == "both":
        cores = [Core.app, Core.net]
    else:
        cores = [Core[options.core]]

    if options.device is None:
        devices = []
    elif options.device == "both":
        devices = [AudioDevice.gateway, AudioDevice.headset]
    else:
        devices = [AudioDevice[options.device]]

    options.build = BuildType[options.build] if options.build else None

    options.only_reboot = SelectFlags.TBD if options.only_reboot else SelectFlags.NOT

    boards_snr_connected = __find_snr()
    if not boards_snr_connected:
        print("No snrs connected")

    # Update device list
    # This JSON file should be altered by the developer.
    # Then run git update-index --skip-worktree FILENAME to avoid changes
    # being pushed
    with USER_CONFIG.open() as f:
        dev_arr = json.load(f)
    device_list = [
        DeviceConf(
            nrf5340_audio_dk_snr=dev["nrf5340_audio_dk_snr"],
            channel=Channel[dev["channel"]],
            snr_connected=(dev["nrf5340_audio_dk_snr"]
                           in boards_snr_connected),
            recover_on_fail=options.recover_on_fail,
            nrf5340_audio_dk_dev=AudioDevice[dev["nrf5340_audio_dk_dev"]],
            cores=cores,
            devices=devices,
            _only_reboot=options.only_reboot,
        )
        for dev in dev_arr
    ]

    __print_dev_conf(device_list)

    # Initialization step finsihed
    # Reboot step start

    if options.only_reboot == SelectFlags.TBD:
        program_threads_run(device_list, options.mcuboot, sequential=options.sequential_prog)
        __finish(device_list)

    # Reboot step finished
    # Build step start

    if options.build is not None:
        print("Invoking build step")
        build_configs = []
        if Core.app in cores:
            if AudioDevice.headset in devices:
                build_configs.append(
                    BuildConf(
                        core=Core.app,
                        device=AudioDevice.headset,
                        pristine=options.pristine,
                        build=options.build,
                    )
                )
            if AudioDevice.gateway in devices:
                build_configs.append(
                    BuildConf(
                        core=Core.app,
                        device=AudioDevice.gateway,
                        pristine=options.pristine,
                        build=options.build,
                    )
                )
        if Core.net in cores:
            print("Net core uses precompiled hex")

        for build_cfg in build_configs:
            __build_module(build_cfg, options)

    # Build step finished
    # Program step start

    if options.program:
        for dev in device_list:
            if dev.snr_connected:
                __populate_hex_paths(dev, options)
        program_threads_run(device_list, options.mcuboot, sequential=options.sequential_prog)

    # Program step finished

    __finish(device_list)


if __name__ == "__main__":
    __main()
