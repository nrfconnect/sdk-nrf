#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

""" Tools to program multiple nRF5340 Audio DKs """

from threading import Thread
from os import system, path
from typing import List
from nrf5340_audio_dk_devices import DeviceConf, SelectFlags, AudioDevice
import shutil

MEM_ADDR_UICR_SNR = 0x00FF80F0
MEM_ADDR_UICR_CH = 0x00FF80F4


def get_prog_tool():
    """Return the programming tool to use: 'nrfutil' if available, else 'nrfjprog'"""
    return 'nrfutil' if shutil.which('nrfutil') else 'nrfjprog'


def build_cmd(tool, action, dev, core=None, hex_path=None):
    """Build the correct command for programming, recovery, or reset."""
    if tool == 'nrfutil':
        if action == 'program':
            if core == 'net':
                return (f"nrfutil.exe device program --core network --serial-number {dev.nrf5340_audio_dk_snr} "
                        f"--firmware {hex_path} --family nrf53")
            elif core == 'app':
                return (f"nrfutil.exe device program --core application --serial-number {dev.nrf5340_audio_dk_snr} "
                        f"--firmware {hex_path} --family nrf53")
        elif action == 'recover':
            if core == 'net':
                return (f"nrfutil.exe device recover --core network --serial-number {dev.nrf5340_audio_dk_snr}")
            elif core == 'app':
                return (f"nrfutil.exe device recover --core application --serial-number {dev.nrf5340_audio_dk_snr}")
            else:
                # nrfutil does not support recover for CP_APPLICATION and CP_NETWORK
                # so we use the generic command
                return (f"nrfutil.exe device recover  --serial-number {dev.nrf5340_audio_dk_snr}")
        elif action == 'reset':
            return f"nrfutil.exe device reset --serial-number {dev.nrf5340_audio_dk_snr}"
        else:
            exit(1, "Invalid action")
    else:  # nrfjprog
        if action == 'program':
            if core == 'net':
                return (f"nrfjprog --program {hex_path}  -f NRF53  -q "
                        f"--snr {dev.nrf5340_audio_dk_snr} --sectorerase --coprocessor CP_NETWORK")
            elif core == 'app':
                return (f"nrfjprog --program {hex_path} -f NRF53  -q "
                        f"--snr {dev.nrf5340_audio_dk_snr} --chiperase --coprocessor CP_APPLICATION")
        elif action == 'recover':
            return (f"nrfjprog --recover --coprocessor {core} --snr {dev.nrf5340_audio_dk_snr}")
        elif action == 'reset':
            return f"nrfjprog -r --snr {dev.nrf5340_audio_dk_snr}"
        else:
            exit(1, "Invalid action")
    return None


def __populate_uicr(dev):
    """Program UICR in device with information from JSON file"""
    tool = get_prog_tool()
    if dev.nrf5340_audio_dk_dev == AudioDevice.headset:
        if tool == 'nrfutil':
            cmd = (f"nrfutil.exe device write --serial-number {dev.nrf5340_audio_dk_snr} --address {MEM_ADDR_UICR_CH} --value {dev.channel.value}")
            print("Programming UICR")
            ret_val = system(cmd)
            if ret_val:
                return False

        else:
            cmd = (f"nrfjprog --memwr {MEM_ADDR_UICR_CH} --val {dev.channel.value} "
                   f"--snr {dev.nrf5340_audio_dk_snr}")
            print("Programming UICR")
            ret_val = system(cmd)
            if ret_val:
                return False
    if tool == 'nrfutil':
        cmd = (f"nrfutil.exe device write --serial-number {dev.nrf5340_audio_dk_snr} --address {MEM_ADDR_UICR_SNR} --value {dev.nrf5340_audio_dk_snr}")
        ret_val = system(cmd)
        if ret_val:
            return False
        else:
            return True
    else:
        cmd = (f"nrfjprog --memwr {MEM_ADDR_UICR_SNR} --val {dev.nrf5340_audio_dk_snr} "
               f"--snr {dev.nrf5340_audio_dk_snr}")
        ret_val = system(cmd)
        if ret_val:
            return False
        else:
            return True


def _program_cores(dev: DeviceConf) -> int:
    tool = get_prog_tool()
    if dev.core_net_programmed == SelectFlags.TBD:
        if not path.isfile(dev.hex_path_net):
            print(f"NET core hex not found. Built only for APP core. {dev.hex_path_net}")
        else:
            print(f"Programming net core on: {dev}")
            cmd = build_cmd(tool, 'program', dev, core='net', hex_path=dev.hex_path_net)
            ret_val = system(cmd)
            if ret_val != 0:
                if not dev.recover_on_fail:
                    dev.core_net_programmed = SelectFlags.FAIL
                return ret_val
            else:
                dev.core_net_programmed = SelectFlags.DONE

    if dev.core_app_programmed == SelectFlags.TBD:
        if not path.isfile(dev.hex_path_app):
            print(f"APP core hex not found. Built only for NET core. {dev.hex_path_app}")
            return 1
        else:
            print(f"Programming app core on: {dev}")
            cmd = build_cmd(tool, 'program', dev, core='app', hex_path=dev.hex_path_app)
            ret_val = system(cmd)
            if ret_val != 0:
                if not dev.recover_on_fail:
                    dev.core_app_programmed = SelectFlags.FAIL
                return ret_val
            else:
                dev.core_app_programmed = SelectFlags.DONE

        # Populate UICR data matching the JSON file
        if not __populate_uicr(dev):
            dev.core_app_programmed = SelectFlags.FAIL
            return 1

    if dev.core_net_programmed != SelectFlags.NOT or dev.core_app_programmed != SelectFlags.NOT:
        print(f"Resetting {dev}")
        cmd = build_cmd(tool, 'reset', dev)
        ret_val = system(cmd)
        if ret_val != 0:
            return ret_val
    return 0


def _recover(dev: DeviceConf):
    tool = get_prog_tool()
    print(f"Recovering device: {dev}")
    for coprocessor in ['CP_NETWORK', 'CP_APPLICATION']:
        cmd = build_cmd(tool, 'recover', dev, core=coprocessor)
        ret_val = system(cmd)
        if ret_val != 0:
            if coprocessor == 'CP_NETWORK':
                dev.core_net_programmed = SelectFlags.FAIL
            else:
                dev.core_app_programmed = SelectFlags.FAIL


def __program_thread(dev: DeviceConf):
    tool = get_prog_tool()
    if dev.only_reboot == SelectFlags.TBD:
        print(f"Resetting {dev}")
        cmd = build_cmd(tool, 'reset', dev)
        ret_val = system(cmd)
        dev.only_reboot = SelectFlags.FAIL if ret_val else SelectFlags.DONE
        return

    return_code = _program_cores(dev)
    if return_code != 0 and dev.recover_on_fail:
        _recover(dev)
        _program_cores(dev)


def program_threads_run(devices_list: List[DeviceConf], sequential: bool = False):
    """Program devices in parallel"""
    threads = []
    # First program net cores if applicable
    for dev in devices_list:
        if not dev.snr_connected:
            dev.only_reboot = SelectFlags.NOT
            dev.core_app_programmed = SelectFlags.NOT
            dev.core_net_programmed = SelectFlags.NOT
            continue
        thread = Thread(target=__program_thread, args=(dev,))
        threads.append(thread)
        thread.start()
        if sequential:
            thread.join()

    for thread in threads:
        thread.join()

    threads.clear()
