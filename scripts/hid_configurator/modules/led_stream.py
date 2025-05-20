#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import struct
import random

from NrfHidDevice import EVENT_DATA_LEN_MAX

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


def validate_params(freq, led_id):
    valid_params = False

    if freq <= 0:
        print('Frequency has to be greater than zero Hz')
    elif led_id < 0:
        print('LED ID cannot be less than zero')
    else:
        valid_params = True

    return valid_params


def led_send_single_step(dev, step, led_id):
    # Chosen data layout for struct is defined using format string.
    event_data = struct.pack('<BBBHHB', step.r, step.g, step.b,
                             step.substep_count, step.substep_time, led_id)

    success = dev.config_set('led_stream', 'set_led_effect', event_data,
                             poll_interval=0.001)

    return success


def fetch_free_steps_buffer_info(dev, led_id):
    success, fetched_data = dev.config_get('led_stream', 'get_leds_state',
                                           poll_interval=0.001)

    if (not success) or (fetched_data is None):
        return False, (None, None)

    # Chosen data layout for struct is defined using format string.
    fmt_initialized = '?'
    fmt_single_led = 'B'
    led_cnt = len(fetched_data) - 1

    fmt = '<' + fmt_initialized + fmt_single_led * led_cnt

    assert struct.calcsize(fmt) <= EVENT_DATA_LEN_MAX

    if len(fetched_data) != struct.calcsize(fmt):
        return False, (None, None)

    data_unpacked = struct.unpack(fmt, fetched_data)

    if led_id >= led_cnt:
        print('Unsupported LED ID')
        return False, (None, None)

    return success, (data_unpacked[0], data_unpacked[led_id + 1])


def send_continuous_led_stream(dev, led_id, freq, substep_cnt = 10):
    if not validate_params(freq, led_id):
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
            success, (ready, free) = fetch_free_steps_buffer_info(dev, led_id)

            if not success:
                break

            if not ready:
                print('LEDs are not ready')
                break

            while free > 0:
                # Send steps with random color and predefined duration
                step.generate_random_color()

                success = led_send_single_step(dev, step, led_id)

                if not success:
                    break

                free -= 1
    except Exception as e:
        print(e)
    except KeyboardInterrupt as e:
        pass

    print('LED stream ended')
