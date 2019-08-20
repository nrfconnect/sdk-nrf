#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import random
import time

from configurator_core import fetch_free_steps_buffer_info, led_send_single_step
from configurator_core import Step

MS_PER_SEC = 1000


def send_continuous_led_stream(dev, recipient, led_id, freq, substep_cnt = 10):
    try:
        # LED stream ends on user request (Ctrl+C) or when an error occurrs.
        while True:
            success, (free, ready) = fetch_free_steps_buffer_info(dev, recipient, led_id)

            if not success:
                return

            if not ready:
                print('LEDs are not ready')
                return

            while free > 0:
                # Send steps with random color and defined duration
                step = Step(
                    r = random.randint(0, 255),
                    g = random.randint(0, 255),
                    b = random.randint(0, 255),
                    substep_count = substep_cnt,
                    substep_time =  MS_PER_SEC // (freq * substep_cnt)
                )
                success = led_send_single_step(dev, recipient, step, led_id)

                if not success:
                    return

                free -= 1
    except:
        pass

if __name__ == '__main__':
    print("Please run configurator_cli.py or gui.py to start application")
