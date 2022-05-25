#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""
Settings for building and flashing nRF5340 Audio DK for different targets.
"""
from dataclasses import dataclass


@dataclass
class SelectFlags:
    """Holds the available status flags"""
    NOT: "str" = "Not selected"
    TBD: "str" = "Selected TBD"
    DONE: "str" = "Selected done"
    FAIL: "str" = "Selected ERR"


@dataclass
class DeviceConf:
    """This config is populated according to connected SEGGER serial numbers
    (snr) and command line arguments"""
    nrf5340_audio_dk_snr: int = 0
    nrf5340_audio_dk_snr_connected: bool = False
    nrf5340_audio_dk_device: str = ""
    channel: str = ""
    only_reboot: str = SelectFlags.NOT
    hex_path_app: str = ""
    core_app_programmed: str = SelectFlags.NOT
    hex_path_net: str = ""
    core_net_programmed: str = SelectFlags.NOT

    def __init__(self, nrf5340_audio_dk_snr, nrf5340_audio_dk_device, channel):
        self.nrf5340_audio_dk_snr = nrf5340_audio_dk_snr
        self.nrf5340_audio_dk_device = nrf5340_audio_dk_device
        self.channel = channel


@dataclass
class BuildConf:
    """Build config"""
    core: str = ""
    device: str = ""
    build: str = ""
    pristine: bool = False

    def __init__(self, core, device, pristine, build):
        self.core = core
        self.device = device
        self.pristine = pristine
        self.build = build
