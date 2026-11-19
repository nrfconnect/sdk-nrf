#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""HCI event classes."""

import struct

# HCI events
EVT_COMMAND_COMPLETE = 0x0E
EVT_COMMAND_STATUS = 0x0F
EVT_LE_META = 0x3E

# HCI LE subevents
SUBEVT_LE_UTP_RECEIVE = 0x36


def parse_tlvs(data: bytes) -> list[tuple[int, bytes]]:
    """Split UTP data into a list of (opcode, value)."""
    tlvs = []
    i = 0
    # Length of the type and length fields.
    tl_length = 3
    while i + tl_length <= len(data):
        opcode, v_length = struct.unpack_from("<BH", data, i)
        tlvs.append((opcode, data[i + tl_length : i + tl_length + v_length]))
        i += tl_length + v_length
    return tlvs


class HciEvent:
    """Base class of the HCI events. Unknown events are instances of this class."""

    def __init__(self, code: int, params: bytes):
        self.code = code
        self.params = params

    @staticmethod
    def parse(code: int, params: bytes) -> "HciEvent":
        """Create the event class instance that matches the event code (and LE subevent)."""
        if code == EVT_COMMAND_COMPLETE:
            return CommandCompleteEvent(code, params)
        if code == EVT_COMMAND_STATUS:
            return CommandStatusEvent(code, params)
        if code == EVT_LE_META:
            if params and params[0] == SUBEVT_LE_UTP_RECEIVE:
                return LeUtpReceiveEvent(code, params)
            return LeMetaEvent(code, params)
        return HciEvent(code, params)


class CommandStatusEvent(HciEvent):
    def __init__(self, code: int, params: bytes):
        super().__init__(code, params)
        self.status, self.num_commands, self.opcode = struct.unpack_from("<BBH", params)


class CommandCompleteEvent(HciEvent):
    def __init__(self, code: int, params: bytes):
        super().__init__(code, params)
        self.num_commands, self.opcode, self.status = struct.unpack_from("<BHB", params)
        self.return_params = params[3:]


class LeMetaEvent(HciEvent):
    """LE meta event without a dedicated class."""

    def __init__(self, code: int, params: bytes):
        super().__init__(code, params)
        self.subevent = params[0]


class LeUtpReceiveEvent(LeMetaEvent):
    def __init__(self, code: int, params: bytes):
        super().__init__(code, params)
        utp_data_length = params[1]
        self.tlvs = parse_tlvs(params[2 : 2 + utp_data_length])
