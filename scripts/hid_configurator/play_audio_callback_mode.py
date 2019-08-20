import colorsys
import sys
import threading
import time
import wave

import numpy as np
import pyaudio

from configurator_core import open_device, get_device_pid, led_send_hz, Color

RATE = 44100
FRAMES_PER_BUFFER = 2048
CHANNELS = 1


def peak_to_hue(peak):
    peak *= 100

    maxPeak = adjust_maxPeak(peak)

    hue = (260 - peak * 400 / maxPeak)
    if hue < 0:
        hue = 0
    if hue > 260:
        hue = 260
    hue /= 360
    return hue


def adjust_maxPeak(peak):
    global maxPeak

    if peak > maxPeak:
        maxPeak = peak

    if maxPeak < 5:
        maxPeak = 5

    print(maxPeak)
    return maxPeak


maxValue = 2 ** 16
maxPeak = 0

color = Color()
dev = open_device('gaming_mouse')
recipient = get_device_pid('gaming_mouse')
led_id = 0

if len(sys.argv) == 2:
    wf = wave.open(sys.argv[1], 'rb')

p = pyaudio.PyAudio()


def callback(in_data, frame_count, time_info, status):
    global color

    if len(sys.argv) == 2:
        in_data = wf.readframes(frame_count)

    data = np.frombuffer(in_data, dtype=np.int16)

    peak = np.abs(np.max(data) - np.min(data)) / maxValue
    print("R:%00.02f" % (peak * 100))

    hue = peak_to_hue(peak)

    r, g, b = colorsys.hsv_to_rgb(hue, 1, 1)

    color = Color(int(r * 255), int(g * 255), int(b * 255))

    threading.Thread(target=led_send_hz, args=(dev, recipient, color, led_id)).start()

    return (data, pyaudio.paContinue)


stream = p.open(format=pyaudio.paInt16,
                channels=CHANNELS,
                rate=RATE,
                output=False,
                input=True,
                stream_callback=callback,
                frames_per_buffer=FRAMES_PER_BUFFER)

stream.start_stream()

while stream.is_active():
    time.sleep(0.1)

stream.stop_stream()
stream.close()

p.terminate()
