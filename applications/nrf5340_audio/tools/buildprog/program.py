#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

""" Tools to program multiple nRF5340 Audio DKs """

from threading import Thread
from os import system
from typing import List
from nrf5340_audio_dk_devices import DeviceConf, SelectFlags, AudioDevice

MEM_ADDR_UICR_SNR = 0x00FF80F0
MEM_ADDR_UICR_CH = 0x00FF80F4


def __populate_UICR(dev):
    """Program UICR in device with information from JSON file"""
    if dev.nrf5340_audio_dk_dev == AudioDevice.headset:
        cmd = f"nrfjprog --memwr {MEM_ADDR_UICR_CH} --val {dev.channel.value} --snr {dev.nrf5340_audio_dk_snr}"
        # Write channel information to UICR
        print("Programming UICR")
        ret_val = system(cmd)

        if ret_val:
            return False

    cmd = f"nrfjprog --memwr {MEM_ADDR_UICR_SNR} --val {dev.nrf5340_audio_dk_snr} --snr {dev.nrf5340_audio_dk_snr}"

    # Write segger nr to UICR
    ret_val = system(cmd)
    if ret_val:
        return False
    else:
        return True


def _program_cores(dev: DeviceConf, mcuboot_type) -> int:
    if dev.core_net_programmed == SelectFlags.TBD:
        print(f"Programming net core on: {dev}")
        cmd = f"nrfjprog --program {dev.hex_path_net}  -f NRF53  -q --snr {dev.nrf5340_audio_dk_snr} --sectorerase --coprocessor CP_NETWORK"
        ret_val = system(cmd)
        if ret_val != 0:
            if not dev.recover_on_fail:
                dev.core_net_programmed = SelectFlags.FAIL
            return ret_val
        else:
            dev.core_net_programmed = SelectFlags.DONE

    if dev.core_app_programmed == SelectFlags.TBD:
        print(f"Programming app core on: {dev}")
        cmd = f"nrfjprog --program {dev.hex_path_app} -f NRF53  -q --snr {dev.nrf5340_audio_dk_snr} --chiperase --coprocessor CP_APPLICATION"
        ret_val = system(cmd)
        if ret_val != 0:
            if not dev.recover_on_fail:
                dev.core_app_programmed =  SelectFlags.FAIL
            return ret_val
        else:
            dev.core_app_programmed = SelectFlags.DONE

        # Populate UICR data matching the JSON file
        if not __populate_UICR(dev):
            dev.core_app_programmed = SelectFlags.FAIL
            return 1

    if dev.core_net_programmed != SelectFlags.NOT or dev.core_app_programmed != SelectFlags.NOT:
        if mcuboot_type =='external':
            print(f"Hard resetting {dev}")
            cmd = f"nrfjprog -p --snr {dev.nrf5340_audio_dk_snr}"
        else:
            print(f"Resetting {dev}")
            cmd = f"nrfjprog -r --snr {dev.nrf5340_audio_dk_snr}"
        ret_val = system(cmd)
        if ret_val != 0:
            return ret_val
    return 0


def _recover(dev: DeviceConf):
    print(f"Recovering device: {dev}")
    ret_val = system(
        f"nrfjprog --recover --coprocessor CP_NETWORK --snr {dev.nrf5340_audio_dk_snr}"
    )
    if ret_val != 0:
        dev.core_net_programmed = SelectFlags.FAIL

    ret_val = system(
        f"nrfjprog --recover --coprocessor CP_APPLICATION --snr {dev.nrf5340_audio_dk_snr}"
    )
    if ret_val != 0:
        dev.core_app_programmed = SelectFlags.FAIL


def __program_thread(dev: DeviceConf, mcuboot_type):
    if dev.only_reboot == SelectFlags.TBD:
        print(f"Resetting {dev}")
        cmd = f"nrfjprog -r --snr {dev.nrf5340_audio_dk_snr}"
        ret_val = system(cmd)
        dev.only_reboot = SelectFlags.FAIL if ret_val else SelectFlags.DONE
        return

    return_code = _program_cores(dev, mcuboot_type)
    if return_code != 0 and dev.recover_on_fail:
        _recover(dev)
        _program_cores(dev, mcuboot_type)


def program_threads_run(devices_list: List[DeviceConf], mcuboot_type, sequential: bool = False):
    """Program devices in parallel"""
    threads = []
    # First program net cores if applicable
    for dev in devices_list:
        if not dev.snr_connected:
            dev.only_reboot = SelectFlags.NOT
            dev.core_app_programmed = SelectFlags.NOT
            dev.core_net_programmed = SelectFlags.NOT
            continue
        thread = Thread(target=__program_thread, args=(dev, mcuboot_type))
        threads.append(thread)
        thread.start()
        if sequential:
            thread.join()

    for thread in threads:
        thread.join()

    threads.clear()
