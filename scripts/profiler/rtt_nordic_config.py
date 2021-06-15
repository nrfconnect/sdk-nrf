#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

RttNordicConfig = {
    'device_snr': None,
    'rtt_up_channel_names':  {
        'Nordic profiler info': 'info',
        'Nordic profiler data': 'data',
    },
    'rtt_down_channel_names': {
        'Nordic profiler command': 'command',
    },
    'ms_per_timestamp_tick': 0.03125,
    'byteorder': 'little',
    'reset_on_start': True,
    'connection_timeout': -1,
    'timestamp_raw_max': 2**32, #timestamp on uC is stored as 32-bit value
    'rtt_read_period': 0.1, #in seconds
    'rtt_read_chunk_size': 64000,
    'rtt_additional_read_thresh': 4096
}
