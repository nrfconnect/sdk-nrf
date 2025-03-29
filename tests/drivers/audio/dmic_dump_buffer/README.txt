This test uses DMIC driver to collect sound from PDM microphone.
Audio samples are output on serial port @921600 bit/s in binary mode.


Follow these steps to collect and analyse sound:

1. Connect PDM microphone (tested with https://www.adafruit.com/product/3492) to PDM_CLK and PDM_DIN as defined in the board overlay.

2. Compile and run dmic_dump_buffer test:
   west build -b nrf54l15dk/nrf54l15/cpuapp --pristine --test-item drivers.audio.dmic_dump_buffer .
   west flash --erase

3. Store output from serial port to a file:
   - press RESET button on DK to stop execution
   - start data capture, f.e.
     picocom -f n -b 921600 /dev/serial/by-id/usb-SEGGER_J-Link_00105xxx-if02 > /home/user/sound_capture.raw
   - release RESET button

4. Collect sound with the microphone. Then, stop data acquisition (f.e. Ctr+a, Ctrl+x)

5. If using picocom, remove text at the beginning and at the end of the file (that is added by picocom).

6. Convert end line symbols
   dos2unix -f /home/user/sound_capture.raw

7. Import raw data to Audacity (Signed 16-bit PCM; little-endian; mono; 16000Hz).

This test is based on https://docs.zephyrproject.org/latest/samples/shields/x_nucleo_iks02a1/microphone/README.html
