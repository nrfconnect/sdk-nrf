# Copyright (c) 2018 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

RttNordicConfig = {
    'device_family': 'NRF52',
    'device_snr' : None,
    'rtt_info_channel': 2,
    'rtt_data_channel': 1,
    'rtt_command_channel': 1,
    'ms_per_timestamp_tick': 0.03125,
    'byteorder': 'little',
    'reset_on_start': False,
    'connection_timeout': -1,
    'timestamp_raw_max': 2**32 #timestamp on uC is stored as 32-bit value
}
