#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import time

from configurator_core import fetch_free_steps_buffer_info, led_send_single_step
from configurator_core import Step

MS_PER_SEC = 1000


def send_continuous_led_stream(dev, recipient, DEVICE_CONFIG, led_id, freq, substep_cnt = 10):
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

    if not valid_params:
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
    except:
        pass

    print('LED stream ended')


if __name__ == '__main__':
    print("Please run configurator_cli.py or gui.py to start application")
