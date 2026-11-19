#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""Nordic vendor specific UTP messages (public ones).

The messages are defined in dragoon/stack/libs/rfphy_test/doc/vs/public. A vendor specific
message is sent as UTP_Set_Vendor_Specific_Data (0xFF, configuration) or UTP_Vendor_Specific_Control
(0xFE, control). Its value is the company ID (2 octets, LE), the vendor opcode (1 octet) and the
opcode specific vendor message.
"""

import struct
from abc import abstractmethod
from enum import IntEnum

from utp.utp_commands import UTP_REPORT_VENDOR_SPECIFIC_DATA, Tlvs, UtpCommand

NORDIC_COMPANY_ID = 0x0059


CATEGORY = "UTP vendor specific commands"


class VsCommand(UtpCommand):
    """Base class of all Nordic vendor specific UTP messages."""

    company_id = NORDIC_COMPANY_ID
    vendor_opcode: int

    @abstractmethod
    def vendor_message(self) -> bytes:
        """The packed opcode specific parameters."""

    def value(self) -> bytes:
        return struct.pack("<HB", self.company_id, self.vendor_opcode) + self.vendor_message()


class VsConfigCommand(VsCommand):
    """Configuration message. The IUT answers with UTP_Accept or UTP_Reject."""

    opcode = 0xFF
    name = "UTP_Set_Vendor_Specific_Data"


class VsControlCommand(VsCommand):
    """Control message. The IUT answers with UTP_Accept, or with a report if report_expected."""

    opcode = 0xFE
    name = "UTP_Vendor_Specific_Control"
    report_expected = False

    def _handle_response(self, response: Tlvs) -> Tlvs:
        opcode, value = response[0]
        if self.report_expected and opcode == UTP_REPORT_VENDOR_SPECIFIC_DATA:
            if value[:3] != struct.pack("<HB", self.company_id, self.vendor_opcode):
                raise RuntimeError(f"{self.name}: report is for another vendor message")
            return response
        return super()._handle_response(response)

    def describe(self, response: Tlvs) -> str:
        opcode, value = response[0]
        if opcode != UTP_REPORT_VENDOR_SPECIFIC_DATA:
            return ""
        if len(value) < 3:
            return "Malformed report"
        company_id, vendor_opcode = struct.unpack_from("<HB", value)
        if company_id != NORDIC_COMPANY_ID:
            return f"Report of company 0x{company_id:04x}, vendor opcode 0x{vendor_opcode:02x}"
        return self.describe_report(value[3:])

    def describe_report(self, message: bytes) -> str:
        """Human readable interpretation of the opcode specific part of a report."""
        return f"Report for vendor opcode 0x{self.vendor_opcode:02x}: {message.hex(' ')}"


class VsSetPacketPayload(VsConfigCommand):
    vendor_opcode = 0x02
    name = "UTP_VS_Set_Packet_Payload"

    class Payload(IntEnum):
        PRBS9 = 0x00
        PATTERN_0X0F = 0x01
        PATTERN_0XAA = 0x02
        PRBS15 = 0x03
        PATTERN_0XFF = 0x04
        PATTERN_0X00 = 0x05
        PATTERN_0XF0 = 0x06
        PATTERN_0X55 = 0x07
        IEEE154_PAYLOAD_1234 = 0xFE
        CONTWAVE = 0xFF

    def __init__(self, payload: Payload):
        self.payload = self.Payload(payload)

    def help(self) -> str:
        return f"Set the vendor specific packet payload to {self.payload.name}"

    def vendor_message(self) -> bytes:
        return struct.pack("<B", self.payload)


class VsSetFrequencyHoppingRange(VsConfigCommand):
    vendor_opcode = 0x03
    name = "UTP_VS_Set_Frequency_Hopping_Range"

    def __init__(self, end_frequency: int):
        """end_frequency: end frequency in MHz, or 0 to disable frequency hopping."""
        self.end_frequency = end_frequency

    def help(self) -> str:
        if not self.end_frequency:
            return "Disable frequency hopping"
        return f"Hop up to {self.end_frequency} MHz"

    def vendor_message(self) -> bytes:
        return struct.pack("<H", self.end_frequency)


class VsSetFrequencyHoppingInterval(VsConfigCommand):
    vendor_opcode = 0x04
    name = "UTP_VS_Set_Frequency_Hopping_Interval"

    def __init__(self, interval_ms: int):
        """interval_ms: maximum time on a frequency, 0 hops after every packet."""
        self.interval_ms = interval_ms

    def help(self) -> str:
        if not self.interval_ms:
            return "Hop to the next frequency after every packet"
        return f"Stay at most {self.interval_ms} ms on a frequency"

    def vendor_message(self) -> bytes:
        return struct.pack("<B", self.interval_ms)


class VsSetTxPowerLevel(VsConfigCommand):
    vendor_opcode = 0x05
    name = "UTP_VS_Set_TX_Power_Level"

    class Level(IntEnum):
        MINIMUM = 0x7FFE
        MAXIMUM = 0x7FFF

    def __init__(self, tx_power_level: Level):
        """tx_power_level: -12700 to 2000 in units of 0.01 dBm, or MINIMUM / MAXIMUM."""
        self.tx_power_level = tx_power_level

    def help(self) -> str:
        if self.tx_power_level in self.Level:
            level = self.Level(self.tx_power_level).name.lower()
            return f"Set TX power to the {level} the IUT supports"
        return f"Set TX power to {self.tx_power_level / 100} dBm"

    def vendor_message(self) -> bytes:
        return struct.pack("<h", self.tx_power_level)


class VsSetFrequencyHoppingDistance(VsConfigCommand):
    vendor_opcode = 0x06
    name = "UTP_VS_Set_Frequency_Hopping_Distance"

    def __init__(self, distance_mhz: int):
        """distance_mhz: distance between consecutive frequencies, must be non-zero."""
        self.distance_mhz = distance_mhz

    def help(self) -> str:
        return f"Hop in steps of {self.distance_mhz} MHz"

    def vendor_message(self) -> bytes:
        return struct.pack("<B", self.distance_mhz)


class VsSetFrequencyHoppingMode(VsConfigCommand):
    vendor_opcode = 0x07
    name = "UTP_VS_Set_Frequency_Hopping_Mode"

    class Mode(IntEnum):
        SEQUENTIAL = 0x00
        RANDOM_PRBS9 = 0x01

    def __init__(self, mode: Mode):
        self.mode = self.Mode(mode)

    def help(self) -> str:
        return f"Set the frequency hopping mode to {self.mode.name}"

    def vendor_message(self) -> bytes:
        return struct.pack("<B", self.mode)


class VsSetFrequency(VsConfigCommand):
    vendor_opcode = 0x08
    name = "UTP_VS_Set_Frequency"

    def __init__(self, frequency: int):
        """frequency: frequency in MHz."""
        self.frequency = frequency

    def help(self) -> str:
        return f"Set the frequency to {self.frequency} MHz"

    def vendor_message(self) -> bytes:
        return struct.pack("<H", self.frequency)


class VsSetRssiReporting(VsConfigCommand):
    vendor_opcode = 0x09
    name = "UTP_VS_Set_RSSI_Reporting"

    class Mode(IntEnum):
        NONE = 0x00
        REGULAR = 0x01
        RAW = 0x02

    def __init__(self, mode: Mode):
        self.mode = self.Mode(mode)

    def help(self) -> str:
        return f"Set RSSI reporting to {self.mode.name}"

    def vendor_message(self) -> bytes:
        return struct.pack("<B", self.mode)


class VsSetTxMode(VsConfigCommand):
    vendor_opcode = 0x0C
    name = "UTP_VS_Set_TX_Mode"

    class Mode(IntEnum):
        REGULAR = 0x00
        BACK_TO_BACK = 0x01

    def __init__(self, mode: Mode):
        self.mode = self.Mode(mode)

    def help(self) -> str:
        return f"Set the transmit mode to {self.mode.name}"

    def vendor_message(self) -> bytes:
        return struct.pack("<B", self.mode)


class VsQuerySupportedOpcodes(VsControlCommand):
    vendor_opcode = 0x01
    name = "UTP_VS_Query_Supported_Opcodes"
    report_expected = True

    def help(self) -> str:
        return "Query the vendor opcodes supported by the IUT"

    def vendor_message(self) -> bytes:
        return b""

    def describe_report(self, message: bytes) -> str:
        names = vendor_opcode_names()
        supported = [
            f"0x{8 * i + bit:02x} {names.get(8 * i + bit, 'unknown')}"
            for i, octet in enumerate(message)
            for bit in range(8)
            if octet >> bit & 1
        ]
        return "Supported vendor opcodes: " + (", ".join(supported) or "none")


class VsReadRssiValues(VsControlCommand):
    vendor_opcode = 0x0A
    name = "UTP_VS_Read_RSSI_Values"
    report_expected = True

    class ValueFormat(IntEnum):
        REGULAR = 0x00
        RAW = 0x01

    def __init__(self, value_format: ValueFormat = ValueFormat.REGULAR):
        self.value_format = self.ValueFormat(value_format)

    def help(self) -> str:
        return f"Read the current RSSI ({self.value_format.name.lower()})"

    def vendor_message(self) -> bytes:
        return struct.pack("<B", self.value_format)

    def describe_report(self, message: bytes) -> str:
        if len(message) != 6:
            return super().describe_report(message)
        frequency, rssi = struct.unpack("<Hi", message)
        return f"RSSI at {frequency} MHz: {rssi / 1000} dBm"


def vendor_opcode_names():
    """Map each vendor opcode to the name of the message that uses it."""
    names = {0x0B: "UTP_VS_Report_RSSI_Values"}
    for base in (VsConfigCommand, VsControlCommand):
        for cls in base.__subclasses__():
            names[cls.vendor_opcode] = cls.name
    return names
