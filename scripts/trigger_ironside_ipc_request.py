# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
Script for triggering IronSide IPC requests via JLink connection.
It does the following:
- takes an (optional) intelhex as input from stdin and programs this to the device
- triggers the IPC request by writing to the correct BELLBOARD.TASK
- waits for the receiving BELLBOARD.EVENT to indicate that ironside has completed the request
- prints the contents of the IPC buffer to stdout if ironside provided a response.

The contents of the intelhex file passed as input must be such that IronSide SE has an IPC
service compatible with the data. See generate_ironside_psa_ipc_request.py for an example.
"""

import argparse
import binascii
import ctypes as c
import io
import sys
from enum import Enum
from time import perf_counter
from typing import List

import pylink
from intelhex import IntelHex


class IronSideCallStatus(Enum):
    IDLE = 0
    SUCCESS = 1
    RSP_ERR_UNKNOWN_STATUS = 2
    RSP_ERR_EXPIRED_STATUS = 3
    RSP_ERR_UNKNOWN_ID = 4
    RSP_ERR_EXPIRED_ID = 5
    REQ = 6

    def __int__(self) -> int:
        return self.value


CallBufferArgs = c.c_uint32 * 7


class CallBuffer(c.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [("status", c.c_uint16), ("id", c.c_uint16), ("args", CallBufferArgs)]

    @property
    def status_enum(self) -> IronSideCallStatus:
        return IronSideCallStatus(self.status)


class IronSideCallResponseError(RuntimeError):
    def __init__(self, rsp_err: IronSideCallStatus) -> None:
        self._rsp_err = rsp_err
        super().__init__(
            f"server responded with error: {self._rsp_err.name} ({self._rsp_err.value})"
        )

    @property
    def rsp_err(self) -> IronSideCallStatus:
        return self._rsp_err


class IronSideCallTimeoutError(TimeoutError): ...


class IronSideCallUnknownStatusError(RuntimeError):
    def __init__(self, status: int) -> None:
        self._status = status
        super().__init__(f"unknown status read from IPC buffer: {self._status}")


class IronSideCall:
    def __init__(
        self,
        jlink: pylink.JLink,
        *,
        tx_task_addr: int,
        rx_event_addr: int,
        buffer_addr: int,
    ) -> None:
        self._jlink = jlink
        self._buffer_addr = buffer_addr
        self._tx_task_addr = tx_task_addr
        self._rx_event_addr = rx_event_addr

    def trigger_request(self) -> None:
        """Trigger the IPC request by setting the TX task"""
        self._jlink.memory_write32(self._tx_task_addr, [0x1])

    def read_response(
        self,
        timeout: int | float | None = 1.0,
    ) -> List[c.c_uint32]:
        timeout_s = timeout if timeout is not None else float("inf")
        has_event = False
        has_data = False
        call_buffer = None

        start = perf_counter()

        while not has_data:
            has_event = False
            while not has_event:
                has_event = bool(self._jlink.memory_read32(self._rx_event_addr, 1)[0])

                if has_event:
                    self._jlink.memory_write32(self._rx_event_addr, [0])
                if (perf_counter() - start) > timeout_s:
                    break

            call_buffer_data = bytes(
                self._jlink.memory_read8(self._buffer_addr, c.sizeof(CallBuffer))
            )
            call_buffer = CallBuffer.from_buffer_copy(call_buffer_data)
            has_data = call_buffer.status not in (
                IronSideCallStatus.IDLE.value,
                IronSideCallStatus.REQ.value,
            )
            if (perf_counter() - start) > timeout_s:
                break

        if not has_event:
            raise IronSideCallTimeoutError(
                f"timed out waiting for RX event at 0x{self._rx_event_addr:09_x}"
            )
        if not has_data:
            raise IronSideCallTimeoutError("timed out waiting for response status")

        assert call_buffer is not None

        try:
            rsp_status = call_buffer.status_enum
        except ValueError:
            raise IronSideCallUnknownStatusError(call_buffer.status)

        if rsp_status != IronSideCallStatus.SUCCESS:
            raise IronSideCallResponseError(rsp_status)

        return [c.c_uint32(a) for a in call_buffer.args]


class IronSideIpcTrigger:
    """Class for triggering IronSide IPC requests"""

    def __init__(self, jlink_serial: int):
        self.jlink = pylink.JLink()
        self.jlink.open(jlink_serial)
        self.jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
        self.jlink.set_speed(4000)
        self.jlink.coresight_configure()
        self.jlink.connect("Cortex-M33")

        self.ironside_call = IronSideCall(
            self.jlink,
            tx_task_addr=0x5F09_9030, # CPUSEC.BELLBOARD.TASK[12]
            rx_event_addr=0x5F09_A100, # CPUAPP.BELLBOARD.EVENT[0]
            buffer_addr=0x2F88_FB80, # Static location of IPC buffer
        )

    def trigger_and_read_response(self) -> List[c.c_uint32]:
        """
        Trigger an IronSide IPC request and read the response.

        Returns:
            List of response arguments
        """

        self.ironside_call.trigger_request()
        return self.ironside_call.read_response()

    def close(self):
        """Close the JLink connection"""
        self.jlink.close()


def program_hex_data(trigger: IronSideIpcTrigger, hex_data: str) -> None:
    """
    Program Intel HEX data to the device using the JLink connection.

    Args:
        trigger: IronSideIpcTrigger instance with active JLink connection
        hex_data: Intel HEX format data as a string

    Raises:
        SystemExit: If Intel HEX data is empty or invalid
    """
    # Parse Intel HEX data
    ih = IntelHex()
    hex_io = io.StringIO(hex_data)
    ih.loadhex(hex_io)

    if len(ih) == 0:
        print("Error: Intel HEX data is empty", file=sys.stderr)
        sys.exit(1)

    print("Halting CPU before flashing...")
    trigger.jlink.halt()

    # Flash each segment in the Intel HEX data
    segments = ih.segments()
    print(f"Found {len(segments)} segment(s) in Intel HEX data")

    for i, (start_addr, end_addr) in enumerate(segments):
        segment_size = end_addr - start_addr
        print(
            f"Flashing segment {i+1}/{len(segments)}: 0x{start_addr:08X} - 0x{end_addr-1:08X} ({segment_size} bytes)"
        )

        # Extract binary data for this segment
        segment_data = ih.tobinarray(start=start_addr, end=end_addr - 1)

        # Convert numpy array to list of bytes for pylink
        data_bytes = list(segment_data.tobytes())

        # Flash this segment
        trigger.jlink.flash(data_bytes, addr=start_addr)
        print(f"  Successfully flashed segment {i+1}")

    print("All segments flashed successfully")


def print_response_result(response_args: List[c.c_uint32]):
    """Print the IPC response result to stdout in hex format"""
    print("IPC Response:")
    print("=============")

    for i, arg in enumerate(response_args):
        print(f"  arg[{i}]: 0x{arg.value:08X} ({arg.value})")

    # Print as continuous hex string like generate_ironside_psa_ipc_request.py
    hex_bytes = []
    for arg in response_args:
        # Convert each uint32 to 4 bytes in little endian format
        byte_data = arg.value.to_bytes(4, byteorder="little")
        hex_bytes.extend(byte_data)

    if hex_bytes:
        hex_string = binascii.hexlify(bytes(hex_bytes)).decode("utf-8").upper()
        print(f"\nHex data: {hex_string}")


def main():
    parser = argparse.ArgumentParser(
        description="Trigger IronSide IPC requests via JLink. Reads Intel HEX data from stdin if available.",
        allow_abbrev=False,
    )

    parser.add_argument(
        "serial_number", type=int, help="Serial number of the JLink device"
    )

    args = parser.parse_args()

    trigger = IronSideIpcTrigger(args.serial_number)

    # Check if there's data on stdin
    if not sys.stdin.isatty():
        print("Reading Intel HEX data from stdin...")
        program_hex_data(trigger, sys.stdin.read())

    response = trigger.trigger_and_read_response()
    print_response_result(response)

    trigger.close()


if __name__ == "__main__":
    main()
