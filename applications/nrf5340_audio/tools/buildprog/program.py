#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

""" Tools to program multiple nRF5340 Audio DKs """

from os import path, system
from threading import Thread
import time

from nrf5340_audio_dk_devices import AudioDevice, DeviceConf, Location, SelectFlags

MEM_ADDR_UICR_SNR = 0x00FF80F0
MEM_ADDR_UICR_LOC = 0x00FF80F4
# Delay between starting/halting devices. Should be enough to deplete audio buffers
# This will give a better experience than killing a headset device mid-stream,
# which may cause audible artefacts.
INTER_KIT_DELAY_SEC = 0.2

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
        cmd = f"nrfutil device write --serial-number {dev.nrf5340_audio_dk_snr} --address {MEM_ADDR_UICR_LOC} --value {locations_to_bitfield(dev.location)}"
        # Write location information to UICR
        print_location_labels(dev.location)
        ret_val = system(cmd)

        if ret_val:
            return False
    cmd = f"nrfutil device write --serial-number {dev.nrf5340_audio_dk_snr} --address {MEM_ADDR_UICR_SNR} --value {dev.nrf5340_audio_dk_snr}"

    ret_val = system(cmd)
    if ret_val:
        print(f"Failed to write SNR UICR for device {dev}")
        return ret_val

    return 0


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

    # Sort so gateways come first. If a headset is halted mid-stream, there may be audible artefacts.
    devices_sorted_for_halt = sorted(devices_list, key=lambda dev: dev.nrf5340_audio_dk_dev != AudioDevice.gateway)
    # Halt all devices before programming
    for dev in devices_sorted_for_halt:
        if dev.snr_connected and SelectFlags.TBD in (dev.core_app_programmed, dev.core_net_programmed, dev.only_reboot):
            cmd = f"nrfutil device halt --serial-number {dev.nrf5340_audio_dk_snr} --family nrf53"
            ret_val = system(cmd)
            if ret_val != 0:
                print(f"Failed to halt device {dev}")
            time.sleep(INTER_KIT_DELAY_SEC)

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

    # Wait for all threads to finish (all kits flashed) before starting devices by writing UICR
    # Start headsets first, as these will advertise and make it easier to sniff packets
    # before devices connect

    sorted_device_list = sorted(devices_list, key=lambda dev: dev.nrf5340_audio_dk_dev != AudioDevice.headset)

    for dev in sorted_device_list:
        # If reset only, this step shall not be executed
        if dev.snr_connected and dev.only_reboot == SelectFlags.NOT:
            time.sleep(INTER_KIT_DELAY_SEC)
            # Populate UICR data matching the JSON file
            if __populate_uicr(dev) != 0:
                dev.core_app_programmed = SelectFlags.FAIL
            # Reset to start application.
            # Note that due to an issue in nrfutil (NRFU-1747), the UICR populate
            # step may also start the device. This will likely cause an extra start and reset.
            cmd = f"nrfutil device reset --serial-number {dev.nrf5340_audio_dk_snr} --family nrf53"
            ret_val = system(cmd)
            if ret_val != 0:
                print(f"Failed to reset device {dev}")
