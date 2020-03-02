#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import struct
import random

from configurator_core import TYPE_FIELD_POS, GROUP_FIELD_POS, MOD_FIELD_POS
from configurator_core import EVENT_GROUP_LED_STREAM
from configurator_core import EVENT_DATA_LEN_MAX

from configurator_core import exchange_feature_report

LED_STREAM_DATA = 0x0
MS_PER_SEC = 1000


class Step:
    def __init__(self, r, g, b, substep_count, substep_time):
        self.r = r
        self.g = g
        self.b = b
        self.substep_count = substep_count
        self.substep_time = substep_time

    def generate_random_color(self):
        self.r = random.randint(0, 255)
        self.g = random.randint(0, 255)
        self.b = random.randint(0, 255)


def validate_params(DEVICE_CONFIG, freq, led_id):
    valid_params = False

    if freq <= 0:
        print('Frequency has to be greater than zero Hz')
    elif DEVICE_CONFIG['stream_led_cnt'] == 0:
        print('Device does not support LED stream')
    elif led_id < 0:
        print('LED ID cannot be less than zero')
    elif led_id >= DEVICE_CONFIG['stream_led_cnt']:
        print('LED with selected ID is not supported on selected device')
    else:
        valid_params = True

    return valid_params


def led_send_single_step(dev, recipient, step, led_id):
    event_id = (EVENT_GROUP_LED_STREAM << GROUP_FIELD_POS) \
               | (LED_STREAM_DATA << TYPE_FIELD_POS)

    # Chosen data layout for struct is defined using format string.
    event_data = struct.pack('<BBBHHB', step.r, step.g, step.b,
                             step.substep_count, step.substep_time, led_id)

    success = exchange_feature_report(dev, recipient, event_id,
                                      event_data, False,
                                      poll_interval=0.001)

    return success


def fetch_free_steps_buffer_info(dev, recipient, led_id):
    event_id = (EVENT_GROUP_LED_STREAM << GROUP_FIELD_POS) \
               | (led_id << MOD_FIELD_POS)

    success, fetched_data = exchange_feature_report(dev, recipient,
                                                    event_id, None, True,
                                                    poll_interval=0.001)

    if (not success) or (fetched_data is None):
        return False, (None, None)

    # Chosen data layout for struct is defined using format string.
    fmt = '<B?'
    assert struct.calcsize(fmt) <= EVENT_DATA_LEN_MAX

    if len(fetched_data) != struct.calcsize(fmt):
        return False, (None, None)

    return success, struct.unpack(fmt, fetched_data)


def send_continuous_led_stream(dev, recipient, DEVICE_CONFIG, led_id, freq, substep_cnt = 10):
    if not validate_params(DEVICE_CONFIG, freq, led_id):
        return

    try:
        # LED stream ends on user request (Ctrl+C) or when an error occurrs.
        step = Step(
            r = 0,
            g = 0,
            b = 0,
            substep_count = substep_cnt,
            substep_time =  MS_PER_SEC // (freq * substep_cnt)
        )

        print('LED stream started, press Ctrl+C to interrupt')
        while True:
            success, (free, ready) = fetch_free_steps_buffer_info(dev, recipient, led_id)

            if not success:
                break

            if not ready:
                print('LEDs are not ready')
                break

            while free > 0:
                # Send steps with random color and predefined duration
                step.generate_random_color()

                success = led_send_single_step(dev, recipient, step, led_id)

                if not success:
                    break

                free -= 1
    except Exception as e:
        print(e)

    print('LED stream ended')
