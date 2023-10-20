#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""
Settings for building and flashing nRF5340 Audio DK for different targets.
"""
from dataclasses import InitVar, dataclass, field
from enum import auto, Enum
from pathlib import Path
from typing import List


class SelectFlags(str, Enum):
    """Holds the available status flags"""

    NOT = "Not selected"
    TBD = "Selected"
    DONE = "Done"
    FAIL = "Failed"


class Core(str, Enum):
    app = "app"
    net = "network"
    both = "both"


class AudioDevice(str, Enum):
    headset = "headset"
    gateway = "gateway"
    both = "both"


class BuildType(str, Enum):
    release = "release"
    debug = "debug"


class Channel(Enum):
    # Value represents UICR channel
    left = 0
    right = 1
    NA = auto()


class Controller(str, Enum):
    acs_nrf53 = "ACS_nRF53"
    sdc = "SDC"


@dataclass
class DeviceConf:
    """This config is populated according to connected SEGGER serial numbers
    (snr) and command line arguments"""

    # Constructor variables
    nrf5340_audio_dk_snr: int
    channel: Channel
    snr_connected: bool
    nrf5340_audio_dk_dev: AudioDevice
    recover_on_fail: bool

    cores: InitVar[List[Core]]
    devices: InitVar[List[AudioDevice]]
    _only_reboot: InitVar[SelectFlags]
    controller: InitVar[List[Controller]]
    # Post init variables
    only_reboot: SelectFlags = field(init=False, default=SelectFlags.NOT)
    hex_path_app: Path = field(init=False, default=None)
    core_app_programmed: SelectFlags = field(
        init=False, default=SelectFlags.NOT)
    hex_path_net: Path = field(init=False, default=None)
    core_net_programmed: SelectFlags = field(
        init=False, default=SelectFlags.NOT)

    def __post_init__(
        self, cores: List[Core], devices: List[AudioDevice], _only_reboot: SelectFlags, controller: Controller,
    ):
        device_selected = self.nrf5340_audio_dk_dev in devices
        self.only_reboot = _only_reboot if device_selected else SelectFlags.NOT
        self.controller = controller
        if self.only_reboot == SelectFlags.TBD:
            return

        if (Core.app in cores) and device_selected:
            self.core_app_programmed = SelectFlags.TBD
        if (Core.net in cores) and device_selected:
            self.core_net_programmed = SelectFlags.TBD

    def __str__(self):
        str = f"{self.nrf5340_audio_dk_snr} {self.nrf5340_audio_dk_dev.name}"
        if self.nrf5340_audio_dk_dev == AudioDevice.headset:
            str += f" {self.channel.name}"
        return str


@dataclass
class BuildConf:
    """Build config"""

    core: Core
    device: AudioDevice
    build: BuildType
    pristine: bool
    controller: Controller
    child_image: bool
