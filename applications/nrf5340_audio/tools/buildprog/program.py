#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

""" Tools to program multiple nRF5340 Audio DKs """

from threading import Thread
from os import system
from nrf5340_audio_dk_devices import SelectFlags

MEM_ADDR_UICR_SNR = 0x00FF80F0
MEM_ADDR_UICR_CH = 0x00FF80F4

UICR_CHANNEL_LEFT = 0
UICR_CHANNEL_RIGHT = 1


def __populate_UICR(dev):
    """ Program UICR in device with information from JSON file """
    if dev.nrf5340_audio_dk_device == "headset":
        if dev.channel == "left":
            cmd = "nrfjprog --memwr " + str(MEM_ADDR_UICR_CH) + " --val " + \
                   str(UICR_CHANNEL_LEFT) + " --snr " + \
                   str(dev.nrf5340_audio_dk_snr)
        elif dev.channel == "right":
            cmd = "nrfjprog --memwr " + str(MEM_ADDR_UICR_CH) + " --val " + \
                  str(UICR_CHANNEL_RIGHT) + " --snr " + \
                  str(dev.nrf5340_audio_dk_snr)
        else:
            print("Channel: " + dev.channel +
                  " does not equal 'left' or 'right'")
            return False

    if dev.nrf5340_audio_dk_device == "headset":
        # Write channel information to UICR
        print("Programming UICR")
        ret_val = system(cmd)

        if ret_val:
            return False

    cmd = "nrfjprog --memwr " + str(MEM_ADDR_UICR_SNR) + " --val " + \
          str(dev.nrf5340_audio_dk_snr) + " --snr " + \
          str(dev.nrf5340_audio_dk_snr)

    # Write segger nr to UICR
    ret_val = system(cmd)
    if ret_val:
        return False
    else:
        return True


def __program_thread(dev, current_core):
    if dev.only_reboot == SelectFlags.TBD:
        cmd = "nrfjprog -r --snr " + str(dev.nrf5340_audio_dk_snr)
    elif current_core == "net":
        print("Programming " + current_core + " on dev snr: " +
              str(dev.nrf5340_audio_dk_snr))
        cmd = "nrfjprog --program " + dev.hex_path_net + \
              " -f NRF53  -q --snr " + str(dev.nrf5340_audio_dk_snr) + \
              " --sectorerase" + " --coprocessor CP_NETWORK"
    elif current_core == "app":
        print("Programming " + current_core + " on dev snr: " +
              str(dev.nrf5340_audio_dk_snr))
        cmd = "nrfjprog --program " + dev.hex_path_app + \
              " -f NRF53  -q --snr " + str(dev.nrf5340_audio_dk_snr) + \
              " --chiperase" + " --coprocessor CP_APPLICATION"
    else:
        raise Exception("Core definition error")

    ret_val = system(cmd)
    if ret_val and dev.only_reboot == SelectFlags.TBD:
        dev.only_reboot = SelectFlags.FAIL
        return
    elif ret_val and current_core == "net":
        dev.core_net_programmed = SelectFlags.FAIL
        return
    elif ret_val and current_core == "app":
        dev.core_app_programmed = SelectFlags.FAIL
        return

    # Populate UICR data matching the JSON file
    if current_core == "app":
        if not __populate_UICR(dev):
            dev.core_app_programmed = SelectFlags.FAIL
            return

    if dev.only_reboot == SelectFlags.TBD:
        dev.only_reboot = SelectFlags.DONE
    elif current_core == "net":
        dev.core_net_programmed = SelectFlags.DONE
    elif current_core == "app":
        dev.core_app_programmed = SelectFlags.DONE

    # Make sure boards are reset
    cmd = "nrfjprog -r --snr " + str(dev.nrf5340_audio_dk_snr)
    system(cmd)


def program_threads_run(devices_list, sequential=False):
    """ Program devices in parallel"""
    threads = list()
    # First program net cores if applicable
    for dev in devices_list:
        if not dev.nrf5340_audio_dk_snr_connected:
            continue

        if dev.only_reboot == SelectFlags.TBD:
            threads.append(Thread(target=__program_thread, args=(dev, None)))
            threads[-1].start()
            if sequential:
                threads[-1].join()
        elif dev.hex_path_net:
            threads.append(Thread(target=__program_thread, args=(dev, "net")))
            threads[-1].start()
            if sequential:
                threads[-1].join()

    for thread in threads:
        thread.join()

    threads.clear()

    for dev in devices_list:
        if not dev.nrf5340_audio_dk_snr_connected:
            continue

        if dev.only_reboot == SelectFlags.NOT and dev.hex_path_app:
            threads.append(Thread(target=__program_thread, args=(dev, "app")))
            threads[-1].start()
            if sequential:
                threads[-1].join()

    for thread in threads:
        thread.join()
