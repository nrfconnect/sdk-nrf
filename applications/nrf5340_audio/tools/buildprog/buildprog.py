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
import getpass
from pathlib import Path
from colorama import Fore, Style
from prettytable import PrettyTable
from nrf5340_audio_dk_devices import (
    BuildType,
    Location,
    DeviceConf,
    BuildConf,
    AudioDevice,
    SelectFlags,
    Core,
    Transport,
)
from program import program_threads_run


BUILDPROG_FOLDER = Path(__file__).resolve().parent
NRF5340_AUDIO_FOLDER = (BUILDPROG_FOLDER / "../..").resolve()
NRF_FOLDER = (BUILDPROG_FOLDER / "../../../..").resolve()
if os.getenv("AUDIO_KIT_SERIAL_NUMBERS_JSON") is None:
    AUDIO_KIT_SERIAL_NUMBERS_JSON = BUILDPROG_FOLDER / "nrf5340_audio_dk_devices.json"
else:
    AUDIO_KIT_SERIAL_NUMBERS_JSON = Path(
        os.getenv("AUDIO_KIT_SERIAL_NUMBERS_JSON"))
TARGET_BOARD_NRF5340_AUDIO_DK_APP_NAME = "nrf5340_audio_dk/nrf5340/cpuapp"

TARGET_AUDIO_FOLDER = NRF5340_AUDIO_FOLDER
TARGET_AUDIO_BUILD_FOLDER = TARGET_AUDIO_FOLDER / "tools/build"

UNICAST_SERVER_OVERLAY = NRF5340_AUDIO_FOLDER / "unicast_server/overlay-unicast_server.conf"
UNICAST_CLIENT_OVERLAY = NRF5340_AUDIO_FOLDER / "unicast_client/overlay-unicast_client.conf"
BROADCAST_SINK_OVERLAY = NRF5340_AUDIO_FOLDER / "broadcast_sink/overlay-broadcast_sink.conf"
BROADCAST_SOURCE_OVERLAY = NRF5340_AUDIO_FOLDER / "broadcast_source/overlay-broadcast_source.conf"

TARGET_RELEASE_FOLDER = "build_release"
TARGET_DEBUG_FOLDER = "build_debug"

MAX_USER_NAME_LEN = 248 - len('\0')


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
        "location",
    ]

    for device in device_list:
        loc_names = str([loc.name for loc in device.location])
        loc_names = loc_names.replace("[", "").replace("]", "").replace("'", "")
        row = []
        row.append(device.nrf5340_audio_dk_snr)
        color = Fore.GREEN if device.snr_connected else Fore.YELLOW
        row.append(color + str(device.snr_connected) + Style.RESET_ALL)
        row.append(device.nrf5340_audio_dk_dev.value)
        row.append(__print_add_color(device.only_reboot))
        row.append(__print_add_color(device.core_app_programmed))
        row.append(__print_add_color(device.core_net_programmed))
        row.append(loc_names)

        table.add_row(row)
    print(table)


def __build_cmd_get(core: Core, device: AudioDevice, build: BuildType,
                    pristine, options):

    build_cmd = (f"west build {TARGET_AUDIO_FOLDER} "
                 f"-b {TARGET_BOARD_NRF5340_AUDIO_DK_APP_NAME} "
                 f"--sysbuild")

    if core == Core.app:
        build_cmd += " --domain nrf5340_audio"
    elif core == Core.net:
        build_cmd += " --domain ipc_radio"
    else:
        raise Exception("Invalid core!")

    if build == BuildType.debug:
        release_flag = ""
    elif build == BuildType.release:
        release_flag = " -DFILE_SUFFIX=release"
    else:
        raise Exception("Invalid build type!")

    device_flag = ""

    if options.nrf21540:
        device_flag += " -Dnrf5340_audio_SHIELD=nrf21540ek"
        device_flag += " -Dipc_radio_SHIELD=nrf21540ek"
    if options.custom_bt_name is not None and options.user_bt_name:
        raise Exception(
            "User BT name option is invalid when custom BT name is set")
    if options.custom_bt_name is not None:
        custom_bt_name = "_".join(options.custom_bt_name)[
            :MAX_USER_NAME_LEN].upper()
        device_flag += " -DCONFIG_BT_DEVICE_NAME=\\\"" + custom_bt_name + "\\\""
    if options.user_bt_name:
        user_specific_bt_name = (
            "AUDIO_DEV_" + getpass.getuser())[:MAX_USER_NAME_LEN].upper()
        device_flag += " -DCONFIG_BT_DEVICE_NAME=\\\"" + user_specific_bt_name + "\\\""
    if options.transport == Transport.broadcast.name:
        if device == AudioDevice.headset:
            overlay_flag = f" -DEXTRA_CONF_FILE={BROADCAST_SINK_OVERLAY}"
        else:
            overlay_flag = f" -DEXTRA_CONF_FILE={BROADCAST_SOURCE_OVERLAY}"
    else:
        if device == AudioDevice.headset:
            overlay_flag = f" -DEXTRA_CONF_FILE={UNICAST_SERVER_OVERLAY}"
        else:
            overlay_flag = f" -DEXTRA_CONF_FILE={UNICAST_CLIENT_OVERLAY}"

    if os.name == 'nt':
        release_flag = release_flag.replace('\\', '/')
    if pristine:
        build_cmd += " --pristine"

    dest_folder = TARGET_AUDIO_BUILD_FOLDER / options.transport / device / core / build

    return build_cmd, dest_folder, device_flag, release_flag, overlay_flag


def __build_module(build_config, options):
    build_cmd, dest_folder, device_flag, release_flag, overlay_flag = __build_cmd_get(
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
        west_str = west_str + device_flag + release_flag + overlay_flag

    print("Run: " + west_str)

    ret_val = os.system(west_str)

    if ret_val:
        raise Exception("cmake error: " + str(ret_val))


def __find_snr():
    """Rebooting or programming requires connected programmer/debugger"""

    stdout = subprocess.check_output("nrfutil device list", shell=True).decode("utf-8")
    snrs = re.findall(r"^\d+$", stdout, re.MULTILINE)

    if not snrs:
        print("No programmer/debugger connected to PC")

    return list(map(int, snrs))


def __populate_hex_paths(dev, options):
    """Poplulate hex paths where relevant"""

    _, temp_dest_folder, _, _, _ = __build_cmd_get(Core.app, dev.nrf5340_audio_dk_dev, options.build, options.pristine, options)
    dev.hex_path_app = temp_dest_folder / "nrf5340_audio/zephyr/zephyr.hex"

    _, temp_dest_folder, _, _, _ = __build_cmd_get(Core.net, dev.nrf5340_audio_dk_dev, options.build, options.pristine, options)
    dev.hex_path_net = temp_dest_folder / "ipc_radio/zephyr/zephyr.hex"


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
        epilog=("If there exists an environmental variable called \"AUDIO_KIT_SERIAL_NUMBERS_JSON\""
                "which contains the location of a json file,"
                "the program will use this file as a substitute for nrf5340_audio_dk_devices.json"),
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
        "--pristine",
        default=False,
        action="store_true",
        help="Will build cleanly"
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
        help="Run nrfutil sequentially instead of in parallel. This may be required in virtual machines",
    )
    parser.add_argument(
        "-f",
        "--recover_on_fail",
        action="store_true",
        dest="recover_on_fail",
        default=False,
        help="Recover device if programming fails",
    )
    parser.add_argument(
        "--nrf21540",
        action="store_true",
        dest="nrf21540",
        default=False,
        help="Set when using nRF21540 for extra TX power",
    )
    parser.add_argument(
        "-cn",
        "--custom_bt_name",
        nargs='*',
        dest="custom_bt_name",
        default=None,
        help="Use custom Bluetooth device name",
    )
    parser.add_argument(
        "-u",
        "--user_bt_name",
        action="store_true",
        dest="user_bt_name",
        default=False,
        help="Set to generate a user specific Bluetooth device name.\
              Note that this will put the computer user name on air in clear text",
    )
    parser.add_argument(
        "-t",
        "--transport",
        required=True,
        choices=[i.name for i in Transport],
        default=Transport.unicast.name,
        help="Select the transport type",
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
    with AUDIO_KIT_SERIAL_NUMBERS_JSON.open() as f:
        dev_arr = json.load(f)
    device_list = []
    for dev in dev_arr:
        if "channel" in dev:
            print("Using deprecated location format. Convert to using Location enum")
            if dev["channel"] == "left":
                location = [Location.FRONT_LEFT]
            elif dev["channel"] == "right":
                location = [Location.FRONT_RIGHT]
            else:
                print("Invalid location, setting to MONO_AUDIO")
                location = [Location.MONO_AUDIO]
        else:
            location = [Location[name] for name in dev["location"]]

        device = DeviceConf(
            location=location,
            nrf5340_audio_dk_snr=dev["nrf5340_audio_dk_snr"],
            snr_connected=(dev["nrf5340_audio_dk_snr"] in boards_snr_connected),
            recover_on_fail=options.recover_on_fail,
            nrf5340_audio_dk_dev=AudioDevice[dev["nrf5340_audio_dk_dev"]],
            cores=cores,
            devices=devices,
            _only_reboot=options.only_reboot,
        )
        device_list.append(device)

    __print_dev_conf(device_list)

    # Initialization step finsihed
    # Reboot step start

    if options.only_reboot == SelectFlags.TBD:
        program_threads_run(device_list, sequential=options.sequential_prog)
        __finish(device_list)

    # Reboot step finished
    # Build step start

    if options.build is not None:
        print("Invoking build step")
        build_configs = []

        if AudioDevice.headset in devices:
            for c in cores:
                build_configs.append(
                    BuildConf(
                        core=c,
                        device=AudioDevice.headset,
                        pristine=options.pristine,
                        build=options.build,
                    )
                )
        if AudioDevice.gateway in devices:
            for c in cores:
                build_configs.append(
                        BuildConf(
                            core=c,
                            device=AudioDevice.gateway,
                            pristine=options.pristine,
                            build=options.build,
                        )
                    )

        for build_cfg in build_configs:
            __build_module(build_cfg, options)

    # Build step finished
    # Program step start

    if options.program:
        for dev in device_list:
            if dev.snr_connected:
                __populate_hex_paths(dev, options)

        program_threads_run(device_list, sequential=options.sequential_prog)

    # Program step finished

    __finish(device_list)


if __name__ == "__main__":
    __main()
