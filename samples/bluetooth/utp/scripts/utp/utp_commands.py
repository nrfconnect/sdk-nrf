#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""UTP commands (Bluetooth Core, Vol 6, Part F).

Each command is a class that stores its opcode and takes the command
parameters in its constructor. run() sends the command over a serial port and prints
the result.
"""

import struct
import sys
from abc import abstractmethod
from enum import IntEnum

import serial

from utp import transport
from utp.command import Command
from utp.hci_events import LeUtpReceiveEvent

OPCODE_LE_UTP_SEND = 0x20A0

# The (opcode, value) TLVs of an IUT response
Tlvs = list[tuple[int, bytes]]

CATEGORY = "UTP commands"


class UtpResponse(IntEnum):
    """Opcodes of the messages the IUT sends."""

    ACCEPT = 0x12
    REJECT = 0x13
    RESET_ACCEPT = 0x14
    TEST_ENDED = 0x15
    REPORT_SUPPORTED_FEATURES = 0x16
    REPORT_VENDOR_SPECIFIC_DATA = 0xFD


UTP_ACCEPT = UtpResponse.ACCEPT
UTP_REJECT = UtpResponse.REJECT
UTP_RESET_ACCEPT = UtpResponse.RESET_ACCEPT
UTP_TEST_ENDED = UtpResponse.TEST_ENDED
UTP_REPORT_SUPPORTED_FEATURES = UtpResponse.REPORT_SUPPORTED_FEATURES
UTP_REPORT_VENDOR_SPECIFIC_DATA = UtpResponse.REPORT_VENDOR_SPECIFIC_DATA


class UtpCommand(Command):
    """Base class of all UTP commands."""

    @abstractmethod
    def value(self) -> bytes:
        """The packed parameters (the Value field of the TLV)."""

    @property
    def _tlv(self) -> bytes:
        """The TLV: Type (1 octet), Length (2 octets, LE), Value."""
        value = self.value()
        return struct.pack("<BH", self.opcode, len(value)) + value

    def _handle_response(self, response: Tlvs) -> Tlvs:
        """Check the IUT's response, a list of (opcode, value) tuples.

        By default the command must be answered with UTP_Accept. Commands that
        get another response (for example a report) override this. Raises
        RuntimeError for a UTP_Reject or any unexpected response.
        """
        opcode, value = response[0]
        if opcode == UTP_ACCEPT:
            return response
        if opcode == UTP_REJECT and len(value) >= 2:
            raise RuntimeError(f"{self.name} rejected: error code 0x{value[1]:02x}")
        raise RuntimeError(f"{self.name}: expected UTP_Accept (0x12), got 0x{opcode:02x}")

    def describe(self, response: Tlvs) -> str:  # pylint: disable=unused-argument
        """Human readable interpretation of a response, or an empty string."""
        return ""

    def _send_tlv(self, port: serial.Serial) -> Tlvs:
        """Send the command as HCI_LE_UTP_Send and return the (opcode, value) TLVs of the IUT's
        response event."""
        tlv = self._tlv
        status, _ = transport.hci_command(port, OPCODE_LE_UTP_SEND, bytes([len(tlv)]) + tlv)
        if status != 0:
            sys.exit(f"HCI_LE_UTP_Send failed: status 0x{status:02x}")
        while True:
            event = transport.read_event(port)
            if isinstance(event, LeUtpReceiveEvent):
                return event.tlvs

    def run(self, port: serial.Serial) -> Tlvs:
        """Send the command over the serial port and print the response."""
        print(f"Sending {self.name} (0x{self.opcode:02x}): {self.help()}")
        response = self._handle_response(self._send_tlv(port))
        for opcode, value in response:
            if opcode in UtpResponse:
                name = UtpResponse(opcode).name
                print(f"Response: {name} (0x{opcode:02x}), value {value.hex(' ') or '-'}")
        description = self.describe(response)
        if description:
            print(description)
        if response[0][0] == UTP_ACCEPT:
            print("The device accepted the command.")
        return response


class SetRfChannel(UtpCommand):
    opcode = 0x01
    name = "UTP_Set_RF_Channel"

    def __init__(self, rf_channel: int):
        self.rf_channel = rf_channel

    def help(self) -> str:
        return f"Set RF channel to {self.rf_channel} ({2402 + self.rf_channel} MHz)"

    def value(self) -> bytes:
        return struct.pack("<B", self.rf_channel)


class SetPacketPayload(UtpCommand):
    opcode = 0x02
    name = "UTP_Set_Packet_Payload"

    class Payload(IntEnum):
        PRBS9 = 0x00
        REPEATED_11110000 = 0x01
        REPEATED_10101010 = 0x02
        PRBS15 = 0x03
        REPEATED_11111111 = 0x04
        REPEATED_00000000 = 0x05
        REPEATED_00001111 = 0x06
        REPEATED_01010101 = 0x07

    def __init__(self, packet_payload: Payload):
        self.packet_payload = self.Payload(packet_payload)

    def help(self) -> str:
        return f"Set packet payload pattern to {self.packet_payload.name}"

    def value(self) -> bytes:
        return struct.pack("<B", self.packet_payload)


class SetPayloadLength(UtpCommand):
    opcode = 0x03
    name = "UTP_Set_Payload_Length"

    def __init__(self, payload_length: int):
        self.payload_length = payload_length

    def help(self) -> str:
        return f"Set payload length to {self.payload_length} octets"

    def value(self) -> bytes:
        return struct.pack("<H", self.payload_length)


class SetPhy(UtpCommand):
    opcode = 0x04
    name = "UTP_Set_PHY"

    class Phy(IntEnum):
        LE_1M = 0x01
        LE_2M = 0x02
        LE_CODED_S8 = 0x03
        LE_CODED_S2 = 0x04

    def __init__(self, phy: Phy):
        self.phy = self.Phy(phy)

    def help(self) -> str:
        return f"Set PHY to {self.phy.name}"

    def value(self) -> bytes:
        return struct.pack("<B", self.phy)


class SetPacketCount(UtpCommand):
    opcode = 0x0B
    name = "UTP_Set_Packet_Count"

    def __init__(self, packet_count: int):
        self.packet_count = packet_count

    def help(self) -> str:
        count = self.packet_count or "0 (unlimited)"
        return f"Set number of packets to {count}"

    def value(self) -> bytes:
        return struct.pack("<I", self.packet_count)


class StartTest(UtpCommand):
    opcode = 0x10
    name = "UTP_Start_Test"

    class TestType(IntEnum):
        TRANSMITTER = 0x00
        RECEIVER = 0x01

    def __init__(self, test_type: TestType, reset_counters: bool = True):
        self.test_type = self.TestType(test_type)
        self.reset_counters = reset_counters

    def help(self) -> str:
        return f"Start {self.test_type.name.lower()} test (reset counters: {self.reset_counters})"

    def value(self) -> bytes:
        return struct.pack("<BB", self.test_type, int(self.reset_counters))


class StopTest(UtpCommand):
    opcode = 0x11
    name = "UTP_Stop_Test"

    def __init__(self, send_report: bool = False):
        self.send_report = send_report

    def help(self) -> str:
        return f"Stop the running test (send report: {self.send_report})"

    def value(self) -> bytes:
        return struct.pack("<B", int(self.send_report))


class SetCteLength(UtpCommand):
    opcode = 0x06
    name = "UTP_Set_CTE_Length"

    def __init__(self, cte_length: int):
        """cte_length: 0 (no CTE) or 2 to 20, in units of 8 us."""
        self.cte_length = cte_length

    def help(self) -> str:
        if not self.cte_length:
            return "Disable the CTE"
        return f"Set CTE length to {self.cte_length} ({self.cte_length * 8} us)"

    def value(self) -> bytes:
        return struct.pack("<B", self.cte_length)


class SetCteType(UtpCommand):
    opcode = 0x07
    name = "UTP_Set_CTE_Type"

    class CteType(IntEnum):
        AOA = 0x00
        AOD_1US = 0x01
        AOD_2US = 0x02

    def __init__(self, cte_type: CteType):
        self.cte_type = self.CteType(cte_type)

    def help(self) -> str:
        return f"Set CTE type to {self.cte_type.name}"

    def value(self) -> bytes:
        return struct.pack("<B", self.cte_type)


class SetTxPowerLevel(UtpCommand):
    opcode = 0x0C
    name = "UTP_Set_Tx_Power_Level"

    class Level(IntEnum):
        MINIMUM = 0x7E
        MAXIMUM = 0x7F

    def __init__(self, tx_power_level: Level):
        """tx_power_level: -127 to 20 dBm, or MINIMUM / MAXIMUM of what the IUT supports."""
        self.tx_power_level = tx_power_level

    def help(self) -> str:
        if self.tx_power_level in self.Level:
            level = self.Level(self.tx_power_level).name.lower()
            return f"Set TX power to the {level} the IUT supports"
        return f"Set TX power to {self.tx_power_level} dBm"

    def value(self) -> bytes:
        return struct.pack("<b", self.tx_power_level)


class SetOtaExclusionPeriod(UtpCommand):
    opcode = 0x0D
    name = "UTP_Set_OTA_Exclusion_Period"

    def __init__(self, ota_exclusion_period: int):
        """ota_exclusion_period: at least 3."""
        self.ota_exclusion_period = ota_exclusion_period

    def help(self) -> str:
        return f"Set OTA exclusion period to {self.ota_exclusion_period}"

    def value(self) -> bytes:
        return struct.pack("<B", self.ota_exclusion_period)


class QuerySupportedFeatures(UtpCommand):
    opcode = 0x0E
    name = "UTP_Query_Supported_Features"

    def help(self) -> str:
        return "Query the features supported by the IUT"

    def value(self) -> bytes:
        return b""

    def _handle_response(self, response: Tlvs) -> Tlvs:
        """The IUT answers with UTP_Report_Supported_Features instead of UTP_Accept."""
        opcode, _ = response[0]
        if opcode == UTP_REPORT_SUPPORTED_FEATURES:
            return response
        return super()._handle_response(response)


class Reset(UtpCommand):
    opcode = 0x0F
    name = "UTP_Reset"

    def __init__(self, reset_seq_number: int = 0):
        self.reset_seq_number = reset_seq_number

    def help(self) -> str:
        return f"Reset the IUT's UTP configuration (sequence number {self.reset_seq_number})"

    def value(self) -> bytes:
        return struct.pack("<B", self.reset_seq_number)

    def _handle_response(self, response: Tlvs) -> Tlvs:
        """The IUT answers with UTP_Reset_Accept, which echoes the sequence number."""
        opcode, value = response[0]
        if opcode == UTP_RESET_ACCEPT:
            if value != bytes([self.reset_seq_number]):
                raise RuntimeError(f"{self.name}: wrong sequence number in UTP_Reset_Accept")
            return response
        return super()._handle_response(response)
