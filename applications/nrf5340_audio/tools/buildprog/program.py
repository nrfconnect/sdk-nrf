#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

""" Tools to program multiple nRF5340 Audio DKs """

from os import path, system
from threading import Thread

from nrf5340_audio_dk_devices import AudioDevice, DeviceConf, Location, SelectFlags

MEM_ADDR_UICR_SNR = 0x00FF80F0
MEM_ADDR_UICR_CH = 0x00FF80F4

def print_location_labels(locations):
    labels = [loc.label for loc in locations if loc.value != 0]
    if not labels and any(loc.value == 0 for loc in locations):
        labels.append(Location.MONO_AUDIO.label)
    print(" + ".join(labels))

def locations_to_bitfield(locations: list[Location]) -> int:
    bitfield = 0
    for loc in locations:
        bitfield |= loc.value
    return bitfield

def __populate_uicr(dev):
    """Program UICR in device with information from JSON file"""
    if dev.nrf5340_audio_dk_dev == AudioDevice.headset:
        print("Writing UICR with location value: " + str(locations_to_bitfield(dev.location)))
        cmd = f"nrfutil device write --serial-number {dev.nrf5340_audio_dk_snr} --address {MEM_ADDR_UICR_CH} --value {locations_to_bitfield(dev.location)}"
        # Write location information to UICR
        print_location_labels(dev.location)
        ret_val = system(cmd)

        if ret_val:
            return False
    cmd = f"nrfutil device write --serial-number {dev.nrf5340_audio_dk_snr} --address {MEM_ADDR_UICR_SNR} --value {dev.nrf5340_audio_dk_snr}"

    # Write segger nr to UICR
    ret_val = system(cmd)
    return not ret_val


def _program_cores(dev: DeviceConf) -> int:
    if dev.core_net_programmed == SelectFlags.TBD:
        if not path.isfile(dev.hex_path_net):
            print(f"NET core hex not found. Built only for APP core. {dev.hex_path_net}")
        else:
            print(f"Programming net core on: {dev}")
            cmd = (f"nrfutil device program --core network --serial-number {dev.nrf5340_audio_dk_snr} "
                        f"--firmware {dev.hex_path_net} --family nrf53 --options chip_erase_mode=ERASE_RANGES_TOUCHED_BY_FIRMWARE")
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
            cmd = (f"nrfutil device program --core application --serial-number {dev.nrf5340_audio_dk_snr} "
                        f"--firmware {dev.hex_path_app} --family nrf53 --options chip_erase_mode=ERASE_ALL")
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
        cmd = f"nrfutil device reset --serial-number {dev.nrf5340_audio_dk_snr}"
        ret_val = system(cmd)
        if ret_val != 0:
            return ret_val
    return 0


def _recover(dev: DeviceConf):
    print(f"Recovering device: {dev}")
    ret_val = system(
        f"nrfutil device recover --core network --serial-number {dev.nrf5340_audio_dk_snr}"
    )
    if ret_val != 0:
        dev.core_net_programmed = SelectFlags.FAIL

    ret_val = system(
        f"nrfutil device recover --core application --serial-number {dev.nrf5340_audio_dk_snr}"
    )
    if ret_val != 0:
        dev.core_app_programmed = SelectFlags.FAIL


def __program_thread(dev: DeviceConf):
    if dev.only_reboot == SelectFlags.TBD:
        print(f"Resetting {dev}")
        cmd = f"nrfutil device reset --serial-number {dev.nrf5340_audio_dk_snr}"
        ret_val = system(cmd)
        dev.only_reboot = SelectFlags.FAIL if ret_val else SelectFlags.DONE
        return

    return_code = _program_cores(dev)
    if return_code != 0 and dev.recover_on_fail:
        _recover(dev)
        _program_cores(dev)


def program_threads_run(devices_list: list[DeviceConf], sequential: bool = False):
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
