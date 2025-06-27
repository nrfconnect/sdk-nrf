#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""
Settings for building and flashing nRF5340 Audio DK for different targets.
"""
from dataclasses import InitVar, dataclass, field
from enum import Enum
from pathlib import Path
from typing import List


class SelectFlags(str, Enum):
    """Holds the available status flags"""
    NOT = "Not selected"
    TBD = "Selected"
    DONE = "Done"
    FAIL = "Failed"


class Core(str, Enum):
    """SoC core"""
    app = "app"
    net = "net"
    both = "both"


class AudioDevice(str, Enum):
    """Audio device"""
    headset = "headset"
    gateway = "gateway"
    both = "both"


class BuildType(str, Enum):
    """Release or debug build"""
    release = "release"
    debug = "debug"


class Location(Enum):
    """Location as bitfield stored in UICR.
    This is the same as the bt_audio_location enum"""

    MONO_AUDIO = (0, "Mono Audio")
    FRONT_LEFT = (1 << 0, "Front Left")
    FRONT_RIGHT = (1 << 1, "Front Right")
    FRONT_CENTER = (1 << 2, "Front Center")
    LOW_FREQ_EFFECTS_1 = (1 << 3, "Low Frequency Effects 1")
    BACK_LEFT = (1 << 4, "Back Left")
    BACK_RIGHT = (1 << 5, "Back Right")
    FRONT_LEFT_OF_CENTER = (1 << 6, "Front Left of Center")
    FRONT_RIGHT_OF_CENTER = (1 << 7, "Front Right of Center")
    BACK_CENTER = (1 << 8, "Back Center")
    LOW_FREQ_EFFECTS_2 = (1 << 9, "Low Frequency Effects 2")
    SIDE_LEFT = (1 << 10, "Side Left")
    SIDE_RIGHT = (1 << 11, "Side Right")
    TOP_FRONT_LEFT = (1 << 12, "Top Front Left")
    TOP_FRONT_RIGHT = (1 << 13, "Top Front Right")
    TOP_FRONT_CENTER = (1 << 14, "Top Front Center")
    TOP_CENTER = (1 << 15, "Top Center")
    TOP_BACK_LEFT = (1 << 16, "Top Back Left")
    TOP_BACK_RIGHT = (1 << 17, "Top Back Right")
    TOP_SIDE_LEFT = (1 << 18, "Top Side Left")
    TOP_SIDE_RIGHT = (1 << 19, "Top Side Right")
    TOP_BACK_CENTER = (1 << 20, "Top Back Center")
    BOTTOM_FRONT_CENTER = (1 << 21, "Bottom Front Center")
    BOTTOM_FRONT_LEFT = (1 << 22, "Bottom Front Left")
    BOTTOM_FRONT_RIGHT = (1 << 23, "Bottom Front Right")
    FRONT_LEFT_WIDE = (1 << 24, "Front Left Wide")
    FRONT_RIGHT_WIDE = (1 << 25, "Front Right Wide")
    LEFT_SURROUND = (1 << 26, "Left Surround")
    RIGHT_SURROUND = (1 << 27, "Right Surround")

    "Kept for compatibility with old code, use the Location enum instead"
    left = FRONT_LEFT
    right = FRONT_RIGHT
    NA = MONO_AUDIO

    def __new__(cls, value, label):
        obj = object.__new__(cls)
        obj._value_ = value
        obj.label = label
        return obj

class Transport(str, Enum):
    """Transport type"""
    broadcast = "broadcast"
    unicast = "unicast"


@dataclass
class DeviceConf:
    """This config is populated according to connected SEGGER serial numbers
    (snr) and command line arguments"""

    # Constructor variables
    nrf5340_audio_dk_snr: int
    location: list  # Now a list of Locations
    snr_connected: bool
    nrf5340_audio_dk_dev: AudioDevice
    recover_on_fail: bool

    cores: InitVar[List[Core]]
    devices: InitVar[List[AudioDevice]]
    _only_reboot: InitVar[SelectFlags]
    # Post init variables
    only_reboot: SelectFlags = field(init=False, default=SelectFlags.NOT)
    hex_path_app: Path = field(init=False, default=None)
    core_app_programmed: SelectFlags = field(
        init=False, default=SelectFlags.NOT)
    hex_path_net: Path = field(init=False, default=None)
    core_net_programmed: SelectFlags = field(
        init=False, default=SelectFlags.NOT)

    def __post_init__(
        self, cores: List[Core], devices: List[AudioDevice], _only_reboot: SelectFlags,
    ):
        device_selected = self.nrf5340_audio_dk_dev in devices
        self.only_reboot = _only_reboot if device_selected else SelectFlags.NOT
        if self.only_reboot == SelectFlags.TBD:
            return

        if (Core.app in cores) and device_selected:
            self.core_app_programmed = SelectFlags.TBD
        if (Core.net in cores) and device_selected:
            self.core_net_programmed = SelectFlags.TBD

    def __str__(self):
        result = f"{self.nrf5340_audio_dk_snr} {self.nrf5340_audio_dk_dev.name}"
        if self.nrf5340_audio_dk_dev == AudioDevice.headset:
            # Print all location labels if multiple
            if isinstance(self.location, list):
                result += " " + "+".join([loc.label for loc in self.location])
            else:
                result += f" {self.location.name}"
        return result


@dataclass
class BuildConf:
    """Build config"""

    core: Core
    device: AudioDevice
    build: BuildType
    pristine: bool
