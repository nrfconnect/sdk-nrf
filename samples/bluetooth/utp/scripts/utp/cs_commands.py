#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""Channel Sounding test commands (Bluetooth Core spec, HCI LE CS Test and LE CS Test End).

Unlike the UTP commands, these are plain HCI commands. The controller must be built with
overlay-cs.conf (CONFIG_BT_CTLR_CHANNEL_SOUNDING=y).
"""

import struct
from abc import ABC, abstractmethod
from enum import IntEnum

from utp import transport

# LE meta subevent sent when the test has ended
SUBEVT_LE_CS_TEST_END_COMPLETE = 0x33


class CsCommand(ABC):
    """Base class of all Channel Sounding test commands."""

    opcode: int
    name: str
    # Maps constructor parameter names to the IntEnum of their acceptable values
    param_enums: dict = {}
    # LE meta subevent that is sent after the command was accepted, if any
    completion_subevent = None

    @abstractmethod
    def help(self) -> str:
        """Human readable description of the command and its parameters."""

    @abstractmethod
    def params(self) -> bytes:
        """The packed command parameters."""

    def _handle_response(self, status, event):
        """Check the status of the command and of its completion event (if any).

        Raises RuntimeError on a non-zero status.
        """
        if status != 0:
            raise RuntimeError(f"{self.name} failed: status 0x{status:02x}")
        return status

    def send(self, send_hci):
        """Send the command and return the checked status.

        send_hci(opcode, params, completion_subevent) sends one HCI command and
        returns (status, completion event parameters or None).
        """
        status, event = send_hci(self.opcode, self.params(), self.completion_subevent)
        return self._handle_response(status, event)

    def run(self, port):
        """Send the command over the serial port and print the result."""
        print(f"Sending HCI command {self.name} (0x{self.opcode:04x}): {self.help()}")
        status = self.send(lambda *args: transport.hci_command(port, *args))
        print("The controller accepted the command (status 0x00)"
              + (", and the test end completed." if self.completion_subevent else "."))
        return status


class CsTest(CsCommand):
    opcode = 0x2095
    name = "HCI_LE_CS_Test"

    class MainMode(IntEnum):
        MODE_1 = 0x01
        MODE_2 = 0x02
        MODE_3 = 0x03

    class SubMode(IntEnum):
        MODE_1 = 0x01
        MODE_2 = 0x02
        MODE_3 = 0x03
        UNUSED = 0xFF

    class Role(IntEnum):
        INITIATOR = 0x00
        REFLECTOR = 0x01

    class RttType(IntEnum):
        AA_ONLY = 0x00
        SOUNDING_32BIT = 0x01
        SOUNDING_96BIT = 0x02
        RANDOM_32BIT = 0x03
        RANDOM_64BIT = 0x04
        RANDOM_96BIT = 0x05
        RANDOM_128BIT = 0x06

    class CsSyncPhy(IntEnum):
        LE_1M = 0x01
        LE_2M = 0x02
        LE_2M_2BT = 0x03

    class SnrControl(IntEnum):
        SNR_18_DB = 0x00
        SNR_21_DB = 0x01
        SNR_24_DB = 0x02
        SNR_27_DB = 0x03
        SNR_30_DB = 0x04
        NOT_USED = 0xFF

    param_enums = {
        "main_mode_type": MainMode,
        "sub_mode_type": SubMode,
        "role": Role,
        "rtt_type": RttType,
        "cs_sync_phy": CsSyncPhy,
        "snr_control_initiator": SnrControl,
        "snr_control_reflector": SnrControl,
    }

    def __init__(
        self,
        main_mode_type=MainMode.MODE_2,
        sub_mode_type=SubMode.UNUSED,
        main_mode_repetition=0,
        mode_0_steps=3,
        role=Role.INITIATOR,
        rtt_type=RttType.AA_ONLY,
        cs_sync_phy=CsSyncPhy.LE_2M,
        cs_sync_antenna_selection=1,
        subevent_len=5000,
        subevent_interval=16,
        max_num_subevents=0,
        transmit_power_level=0,
        t_ip1_time=30,
        t_ip2_time=30,
        t_fcs_time=50,
        t_pm_time=10,
        t_sw_time=10,
        tone_antenna_config_selection=0,
        cs_enhancements_1=0,
        snr_control_initiator=SnrControl.NOT_USED,
        snr_control_reflector=SnrControl.NOT_USED,
        drbg_nonce=0,
        channel_map_repetition=1,
        override_config=0,
        override_parameters=b"",
    ):
        self.main_mode_type = self.MainMode(main_mode_type)
        self.sub_mode_type = self.SubMode(sub_mode_type)
        self.main_mode_repetition = main_mode_repetition
        self.mode_0_steps = mode_0_steps
        self.role = self.Role(role)
        self.rtt_type = self.RttType(rtt_type)
        self.cs_sync_phy = self.CsSyncPhy(cs_sync_phy)
        self.cs_sync_antenna_selection = cs_sync_antenna_selection
        self.subevent_len = subevent_len
        self.subevent_interval = subevent_interval
        self.max_num_subevents = max_num_subevents
        self.transmit_power_level = transmit_power_level
        self.t_ip1_time = t_ip1_time
        self.t_ip2_time = t_ip2_time
        self.t_fcs_time = t_fcs_time
        self.t_pm_time = t_pm_time
        self.t_sw_time = t_sw_time
        self.tone_antenna_config_selection = tone_antenna_config_selection
        self.cs_enhancements_1 = cs_enhancements_1
        self.snr_control_initiator = self.SnrControl(snr_control_initiator)
        self.snr_control_reflector = self.SnrControl(snr_control_reflector)
        self.drbg_nonce = drbg_nonce
        self.channel_map_repetition = channel_map_repetition
        self.override_config = override_config
        self.override_parameters = bytes(override_parameters)

    def help(self):
        return (
            f"Start a CS test as {self.role.name.lower()}: main {self.main_mode_type.name}, "
            f"sub {self.sub_mode_type.name}, RTT {self.rtt_type.name}, sync PHY "
            f"{self.cs_sync_phy.name}, subevent length {self.subevent_len} us, interval "
            f"{self.subevent_interval} x 0.625 ms, "
            f"{self.max_num_subevents or 'unlimited'} subevents, TX power "
            f"{self.transmit_power_level} dBm"
        )

    def params(self):
        return (
            struct.pack(
                "<8B",
                self.main_mode_type,
                self.sub_mode_type,
                self.main_mode_repetition,
                self.mode_0_steps,
                self.role,
                self.rtt_type,
                self.cs_sync_phy,
                self.cs_sync_antenna_selection,
            )
            + struct.pack("<I", self.subevent_len)[:3]
            + struct.pack(
                "<HBb5BBBBBHBHB",
                self.subevent_interval,
                self.max_num_subevents,
                self.transmit_power_level,
                self.t_ip1_time,
                self.t_ip2_time,
                self.t_fcs_time,
                self.t_pm_time,
                self.t_sw_time,
                self.tone_antenna_config_selection,
                self.cs_enhancements_1,
                self.snr_control_initiator,
                self.snr_control_reflector,
                self.drbg_nonce,
                self.channel_map_repetition,
                self.override_config,
                len(self.override_parameters),
            )
            + self.override_parameters
        )


class CsTestEnd(CsCommand):
    opcode = 0x2096
    name = "HCI_LE_CS_Test_End"
    completion_subevent = SUBEVT_LE_CS_TEST_END_COMPLETE

    def help(self):
        return "End the running CS test"

    def params(self):
        return b""

    def _handle_response(self, status, event):
        super()._handle_response(status, event)
        # The HCI_LE_CS_Test_End_Complete event carries its own status
        if event is not None and len(event) >= 2 and event[1] != 0:
            raise RuntimeError(f"{self.name}: test end failed: status 0x{event[1]:02x}")
        return status
