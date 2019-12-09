#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import time
import pyaudio
import numpy as np
import colorsys
import wave
import threading
import queue

from configurator_core import fetch_free_steps_buffer_info, led_send_single_step
from configurator_core import Step

MS_PER_SEC = 1000


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
    except:
        pass

    print('LED stream ended')


class MusicLedStream():
    def __init__(self, dev, recipient, DEVICE_CONFIG, led_id, freq, filename):
        if not validate_params(DEVICE_CONFIG, freq, led_id):
            raise ValueError("Invalid music LED stream parameters")

        success, (free, ready) = fetch_free_steps_buffer_info(dev, recipient, led_id)
        if not success:
            raise Exception("Device communication problem occurred")

        if not ready:
            raise Exception("LEDs are not ready")

        try:
            self.file = wave.open(filename, 'rb')
        except (wave.Error, FileNotFoundError) as e:
            raise e

        self.dev_params = {
            'max_free' : free,
            'dev' : dev,
            'recipient' : recipient,
            'led_id' : led_id,
        }

        self.send_error_event = threading.Event()
        self.queue = queue.Queue()

        self.p = pyaudio.PyAudio()
        self.stream = self.p.open(format=self.p.get_format_from_width(self.file.getsampwidth()),
                                  channels=self.file.getnchannels(),
                                  rate=self.file.getframerate(),
                                  output=True,
                                  input=False,
                                  stream_callback=self.music_callback,
                                  frames_per_buffer=self.file.getframerate() // freq,
                                  start = False)

        self.led_effects = {
            'BASE_DURATION' : 1 / freq,
            'SUBSTEP_CNT' : 10,
            'sent_cnt' : 0,
            'reminder' : 0,
            'duration_increment' : 0,
            'start_time' : None,
            'out_latency' : self.stream.get_output_latency(),
        }


    @staticmethod
    def peak_to_hue(peak):
        assert peak >= 0 and peak <= 1

        G = 120
        B = 240
        HUE_MAX = 360

        hue = (B - peak * (B - G))
        hue /= HUE_MAX

        return hue

    @staticmethod
    def gen_led_color(data):
        # Cast to prevent overflow
        peak = min(1, np.abs(np.int32(np.max(data)) - np.min(data)) / np.iinfo(np.int16).max)
        hue = MusicLedStream.peak_to_hue(peak)

        return colorsys.hsv_to_rgb(hue, 1, 1)

    def send_led_effects(self):
        while True:
            data = self.queue.get()

            if data == None:
                # LED stream ended
                return

            data = np.frombuffer(data, dtype=np.int16)

            duration = self.led_effects['BASE_DURATION'] + \
                       self.led_effects['reminder'] + \
                       self.led_effects['duration_increment']

            r, g, b = MusicLedStream.gen_led_color(data)

            step = Step(
                r = int(r * 255),
                g = int(g * 255),
                b = int(b * 255),
                substep_count = self.led_effects['SUBSTEP_CNT'],
                substep_time = int(duration * MS_PER_SEC /
                                   self.led_effects['SUBSTEP_CNT'])
            )

            if step.substep_time < 0:
                step.substep_time = 0

            self.led_effects['reminder'] = duration - \
                        (step.substep_count * step.substep_time / MS_PER_SEC)

            success = led_send_single_step(self.dev_params['dev'],
                                           self.dev_params['recipient'],
                                           step,
                                           self.dev_params['led_id'])

            if not success:
                print("Device communication problem")
                send_error_event.set()
                return

            self.led_effects['sent_cnt'] += 1

            success, (free, ready) = fetch_free_steps_buffer_info(self.dev_params['dev'],
                                                    self.dev_params['recipient'],
                                                    self.dev_params['led_id'])

            if not success or not ready:
                self.send_error_event.set()
                if not ready:
                    print("LEDs are not ready")
                else:
                    print("Device communication problem")
                return

            cur_step_speaker = (time.time() - self.led_effects['start_time'] - \
                                self.led_effects['out_latency']) \
                                // self.led_effects['BASE_DURATION']

            if cur_step_speaker < 0:
                # Speaker not ready yet
                continue

            in_buf_speaker = self.led_effects['sent_cnt'] - cur_step_speaker - 1
            in_buf_dev = self.dev_params['max_free'] - free

            DURATION_INCREMENT_CONST_FACTOR = 0.1
            DURATION_INCREMENT_VARIABLE_FACTOR = 0.8

            if in_buf_speaker != 0:
                duration_inc_variable = (in_buf_speaker - in_buf_dev) / \
                            in_buf_speaker * DURATION_INCREMENT_VARIABLE_FACTOR
            else:
                duration_inc_variable = 0

            self.led_effects['duration_increment'] = duration_inc_variable
            if in_buf_speaker > in_buf_dev:
                self.led_effects['duration_increment'] += DURATION_INCREMENT_CONST_FACTOR
            if in_buf_speaker < in_buf_dev:
                self.led_effects['duration_increment'] -= DURATION_INCREMENT_CONST_FACTOR

            self.led_effects['duration_increment'] *= self.led_effects['BASE_DURATION']

    def music_callback(self, in_data, frame_count, time_info, status):
        if status == pyaudio.paOutputUnderflow:
            print("Underflow occurred. Please lower the stream frequency.")
            return (None, pyaudio.paAbort)

        if self.send_error_event.is_set():
            return (None, pyaudio.paAbort)

        data = self.file.readframes(frame_count)
        self.queue.put(data)

        return (data, pyaudio.paContinue)

    def stream_term(self):
        print("Music LED stream ended")

        self.stream.stop_stream()
        self.stream.close()
        self.p.terminate()
        self.file.close()

        # Shutdown thread sending LED events to device
        self.queue.put(None)

    def stream_start(self):
        try:
            print("Music LED stream started. Press Ctrl+C to interrupt.")
            threading.Thread(target=self.send_led_effects).start()
            self.led_effects['start_time'] = time.time()

            self.stream.start_stream()
            while self.stream.is_active():
                time.sleep(0.1)

        except KeyboardInterrupt:
            print("Music LED stream interrupted by user")
            pass

        self.stream_term()


def send_music_led_stream(dev, recipient, DEVICE_CONFIG, led_id, freq, filename):
    try:
        mls = MusicLedStream(dev, recipient, DEVICE_CONFIG, led_id, freq, filename)
        mls.stream_start()
    except Exception as e:
        print(e)


if __name__ == '__main__':
    print("Please run configurator_cli.py or gui.py to start application")
