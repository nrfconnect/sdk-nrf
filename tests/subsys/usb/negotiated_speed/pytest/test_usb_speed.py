# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import logging
import os
import subprocess
import sys
import time

from twister_harness import DeviceAdapter

USB_SAMPLE_ID: str = "NordicSemiconductor USBD MSC sample"
packet_size_vs_speed: dict[int, str] = {
    64: "Full Speed",
    512: "High Speed",
    1024: "Super Speed",
}

logger = logging.getLogger("usb_negotiated_speed")
logger.setLevel(logging.DEBUG)


def excute_command(
    command: str,
) -> str:
    """
    Execute the command in subprocess.check_output()
    :return: command standard output
    """
    logger.debug(command)
    return subprocess.check_output(
        command.split(" "),
        env=os.environ.copy(),
    ).decode(sys.getdefaultencoding())


def test_usb_negotaited_speed(dut: DeviceAdapter):
    """
    Test the USB device negotiated speed
    The nrf54h20 and nrf54lm20 devices
    must attach as High Speed USB device
    """
    # wait for enumeration
    time.sleep(2)

    usb_devices: str = excute_command("lsusb")
    logger.info(usb_devices)
    dut.readlines()

    failure_info: str = f"{USB_SAMPLE_ID} not found in the USB devices"

    assert USB_SAMPLE_ID in usb_devices, failure_info

    vendor_and_dev_id: str = (
        usb_devices.split(USB_SAMPLE_ID)[0].split("ID ")[-1].strip(" ")
    )
    usb_device_info: str = excute_command(f"lsusb -d {vendor_and_dev_id} -v")
    max_packet_size: int = int(
        usb_device_info.split("wMaxPacketSize")[-1]
        .split("bytes")[0]
        .split("1x ")[-1]
        .strip(" ")
    )
    negotiated_speed: str = packet_size_vs_speed[max_packet_size]

    logger.info(f"Negotiated speed: {negotiated_speed}")

    failure_info = "Negotiated USB device speed is not 'High Speed'"
    assert negotiated_speed == "High Speed", failure_info
