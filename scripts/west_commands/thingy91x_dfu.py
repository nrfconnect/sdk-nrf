#!/usr/bin/env python3
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import sys
import usb.core
import usb.util
import argparse
import logging as default_logging
import serial
import serial.tools.list_ports
import os
import time
from west.commands import WestCommand
import west.log
from tempfile import TemporaryDirectory
from zipfile import ZipFile
import re

# Workaround for pyusb on windows
# libusb1 backend is used on all platforms,
# but on windows, the library DLL is not found automatically
if sys.platform == "win32":
    import usb.backend.libusb1
    import libusb
    if usb.backend.libusb1.get_backend() is None:
        usb.backend.libusb1.get_backend(find_library=lambda x : libusb.dll._name)

RECOVER_NRF53 = b"\x8e"  # connectivity_bridge bootloader mode
RECOVER_NRF91 = b"\x8f"  # nrf91 bootloader mode
RESET_NRF53 = b"\x90"  # reset nrf53
RESET_NRF91 = b"\x91"  # reset nrf91


def add_args_to_parser(parser, default_chip=""):
    # define argument to decide whether to recover nrf53 or nrf91
    parser.add_argument(
        "--chip",
        type=str,
        help="bootloader mode to trigger: nrf53 or nrf91",
        default=default_chip,
    )
    parser.add_argument("--vid", type=int, help="vendor id", default=0x1915)
    parser.add_argument("--pid", type=int, help="product id", default=0x910A)
    parser.add_argument("--serial", type=str, help="serial number", default=None)
    parser.add_argument(
        "--debug", action="store_true", help="enable debug logging", default=False
    )


def find_bulk_interface(device, descriptor_string, logging):
    for cfg in device:
        for intf in cfg:
            # Attempt to detach kernel driver (only necessary on Linux)
            if sys.platform == "linux" and device.is_kernel_driver_active(
                intf.bInterfaceNumber
            ):
                try:
                    device.detach_kernel_driver(intf.bInterfaceNumber)
                except usb.core.USBError as e:
                    logging.error(f"Could not detach kernel driver: {str(e)}")
            if usb.util.get_string(device, intf.iInterface) == descriptor_string:
                return intf
    return None

def find_usb_device_posix(vid, pid, logging, serial_number=None):
    if serial_number is not None:
        device = usb.core.find(idVendor=vid, idProduct=pid, serial_number=serial_number)
        if device is None:
            logging.error(f"Device with serial number {serial_number} not found")
            return None, None
        return device, serial_number
    devices = list(usb.core.find(find_all=True, idVendor=vid, idProduct=pid))
    if len(devices) == 1:
        device = devices[0]
        serial_number = usb.util.get_string(device, device.iSerialNumber)
        return device, serial_number
    if len(devices) == 0:
        logging.error("No devices found.")
    else:
        logging.info("Multiple devices found.")
        for device in devices:
            serial_number = usb.util.get_string(device, device.iSerialNumber)
            logging.info(f"Serial Number: {serial_number}")
        logging.warning(
            "Please specify the serial number with the --serial option."
        )
    return None, None

def find_usb_device_windows(vid, pid, logging, serial_number=None):
    devices = list(usb.core.find(find_all=True, idVendor=vid, idProduct=pid))
    if len(devices) == 1:
        coms = serial.tools.list_ports.comports()
        for com in coms:
            if com.vid == vid and com.pid == pid:
                if serial_number is not None and serial_number != com.serial_number:
                    logging.error(
                        f"Device with serial number {serial_number} not found"
                    )
                    return None, None
                return devices[0], com.serial_number
    if len(devices) == 0:
        logging.error("No devices found.")
    else:
        # On Windows, we cannot reliably get the serial number from the device.
        # Therefore, only one device can be connected at a time.
        logging.info("Multiple devices found. Please make sure only one device is connected.")
    return None, None

def find_usb_device(vid, pid, logging, serial_number=None):
    if sys.platform == "win32":
        return find_usb_device_windows(vid, pid, logging, serial_number)
    else:
        return find_usb_device_posix(vid, pid, logging, serial_number)

def trigger_bootloader(vid, pid, chip, logging, reset_only, serial_number=None):

    device, serial_number = find_usb_device(vid, pid, logging, serial_number)

    if device is None:
        return

    if "THINGY91X" not in serial_number:
        logging.warning("Device is already in bootloader mode")
        if chip != "nrf53":
            logging.error(
                "The device is in nRF53 bootloader mode, " + \
                "but you are trying to flash an nRF91 image. " + \
                "Please program the connectivity_bridge firmware first."
            )
            return
        return serial_number

    # Find the bulk interface
    bulk_interface = find_bulk_interface(device, "CMSIS-DAP v2", logging)
    if bulk_interface is None:
        logging.error("Bulk interface not found")
        return

    # Find the out endpoint
    out_endpoint = None
    for endpoint in bulk_interface:
        if usb.util.endpoint_direction(endpoint.bEndpointAddress) == usb.util.ENDPOINT_OUT:
            out_endpoint = endpoint
            break
    if out_endpoint is None:
        logging.error("OUT endpoint not found")
        return

    # Find the in endpoint
    in_endpoint = None
    for endpoint in bulk_interface:
        if usb.util.endpoint_direction(endpoint.bEndpointAddress) == usb.util.ENDPOINT_IN:
            in_endpoint = endpoint
            break
    if in_endpoint is None:
        logging.error("IN endpoint not found")
        return

    if reset_only:
        if chip == "nrf53":
            logging.info("Trying to reset nRF53...")
            try:
                out_endpoint.write(RESET_NRF53)
                logging.debug("Data sent successfully.")
            except usb.core.USBError as e:
                logging.error(f"Failed to send data: {e}")
                return
        else:
            logging.info("Trying to reset nRF91...")
            try:
                out_endpoint.write(RESET_NRF91)
                logging.debug("Data sent successfully.")
            except usb.core.USBError as e:
                logging.error(f"Failed to send data: {e}")
                return
            # Read the response
            try:
                data = in_endpoint.read(in_endpoint.wMaxPacketSize)
                logging.debug(f"Response: {data}")
            except usb.core.USBError as e:
                logging.error(f"Failed to read data: {e}")
        usb.util.dispose_resources(device)
        return serial_number

    # for nrf91, check if the first serial port of the device is available
    # for nrf53, the device will re-enumerate anyway
    if chip == "nrf91":
        found_ports = []
        for port in serial.tools.list_ports.comports():
            if port.serial_number == serial_number:
                found_ports.append(port.device)
        if len(found_ports) != 2:
            logging.error("No serial ports with that serial number found")
            sys.exit(1)
        found_ports.sort()
        try:
            with serial.Serial(found_ports[0], 115200, timeout=1):
                logging.debug("Serial port opened")
        except serial.SerialException as e:
            logging.error(f"Failed to open serial port, do you have a serial terminal open? {e}")
            sys.exit(1)

    # Send the command to trigger the bootloader
    if chip == "nrf53":
        logging.info("Trying to put nRF53 in bootloader mode...")
        try:
            out_endpoint.write(RECOVER_NRF53)
            logging.debug("Data sent successfully.")
        except usb.core.USBError as e:
            logging.error(f"Failed to send data: {e}")
            return
        # Device should be in bootloader mode now, no need to read the response
    else:
        logging.info("Trying to put nRF91 in bootloader mode...")
        try:
            out_endpoint.write(RECOVER_NRF91)
            logging.debug("Data sent successfully.")
        except usb.core.USBError as e:
            logging.error(f"Failed to send data: {e}")
            return
        # Read the response
        try:
            data = in_endpoint.read(in_endpoint.wMaxPacketSize)
            logging.debug(f"Response: {data}")
        except usb.core.USBError as e:
            logging.error(f"Failed to read data: {e}")
    usb.util.dispose_resources(device)
    return serial_number


def perform_dfu(pid, vid, serial_number, image, chip, logging):
    # extract the hexadecimal part of the serial number
    match = re.search(r"(THINGY91X_)?([A-F0-9]+)", serial_number)
    if match is None:
        logging.error("Serial number doesn't match expected format")
        sys.exit(1)
    serial_number_digits = match.group(2)

    if chip == "nrf53":
        port_info = None
        logging.debug("Waiting for device to enumerate...")
        for _ in range(300):
            time.sleep(0.1)
            try:
                com_ports = serial.tools.list_ports.comports()
            except TypeError:
                continue
            for port_info in com_ports:
                if port_info.serial_number is None:
                    continue
                logging.debug(
                    f"Serial port: {port_info.device} has serial number: {port_info.serial_number}"
                )
                if serial_number_digits in port_info.serial_number:
                    logging.debug(f"Serial port: {port_info.device}")
                    serial_number = port_info.serial_number
                    break
            if port_info is None:
                continue
            if "THINGY91X" not in serial_number:
                break
        if port_info is None:
            logging.error("MCUBoot serial port not found")
            sys.exit(1)

    # Assemble DFU update command:
    command = (
        f"nrfutil device program --serial-number {serial_number} --firmware {image}"
    )

    logging.debug(f"Executing command: {command}")
    os.system(command)


def detect_family_from_zip(zip_file, logging):
    with TemporaryDirectory() as tmpdir:
        with ZipFile(zip_file, "r") as zip_ref:
            zip_ref.extractall(tmpdir)
        try:
            with open(os.path.join(tmpdir, "manifest.json")) as manifest:
                manifest_content = manifest.read()
        except FileNotFoundError:
            logging.error("Manifest file not found")
            return None
    if "nrf53" in manifest_content:
        return "nrf53"
    if "nrf91" in manifest_content:
        return "nrf91"
    return None


def main(args, reset_only, logging=default_logging):
    # if logging does not contain .error function, map it to .err
    if not hasattr(logging, "error"):
        logging.debug = logging.dbg
        logging.info = logging.inf
        logging.warning = logging.wrn
        logging.error = logging.err

    # determine chip family
    chip = args.chip
    if hasattr(args, 'image') and args.image is not None:
        # if image is provided, try to determine chip family from image
        chip = detect_family_from_zip(args.image, logging)
        if chip is None:
            logging.error("Could not determine chip family from image")
            sys.exit(1)
    if len(args.chip) > 0 and args.chip != chip:
        logging.error("Chip family does not match image")
        sys.exit(1)
    if chip not in ["nrf53", "nrf91"]:
        logging.error("Invalid chip")
        sys.exit(1)

    serial_number = trigger_bootloader(args.vid, args.pid, chip, logging, reset_only, args.serial)
    if serial_number is not None:
        if reset_only:
            logging.info(f"{chip} on {serial_number} has been reset")
        else:
            logging.info(f"{chip} on {serial_number} is in bootloader mode")
    else:
        sys.exit(1)

    # Only continue if an image file is provided
    if hasattr(args, 'image') and args.image is not None:
        perform_dfu(args.pid, args.vid, serial_number, args.image, chip, logging)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Thingy91X DFU", allow_abbrev=False)
    parser.add_argument("--image", type=str, help="application update file")
    add_args_to_parser(parser)
    args = parser.parse_args()

    default_logging.basicConfig(
        level=default_logging.DEBUG if args.debug else default_logging.INFO
    )

    main(args, reset_only=False, logging=default_logging)


class Thingy91XDFU(WestCommand):
    def __init__(self):
        super(Thingy91XDFU, self).__init__(
            "thingy91x-dfu",
            "Thingy:91 X DFU",
            "Put Thingy:91 X in DFU mode and update using MCUBoot serial recovery.",
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description
        )
        add_args_to_parser(parser)
        parser.add_argument("--image", type=str, help="application update file",
                            default="build/dfu_application.zip")

        return parser

    def do_run(self, args, unknown_args):
        main(args, reset_only=False, logging=west.log)

class Thingy91XReset(WestCommand):
    def __init__(self):
        super(Thingy91XReset, self).__init__(
            "thingy91x-reset",
            "Thingy:91 X Reset",
            "Reset Thingy:91 X.",
        )
    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description
        )
        add_args_to_parser(parser, default_chip="nrf91")

        return parser

    def do_run(self, args, unknown_args):
        main(args, reset_only=True, logging=west.log)
