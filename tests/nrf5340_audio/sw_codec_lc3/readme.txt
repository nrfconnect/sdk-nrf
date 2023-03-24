# LC3 test
This test can only be run on nRF5340 Audio application because the `qemu_cortex_m3` target does not support FPU.

To run the test on target, run the following command, with the COM port (here, `/dev/ttyACM2`) set to the APP port of your development kit:

zephyr/scripts/twister --device-testing --device-serial /dev/ttyACM2 --platform nrf5340dk_nrf5340_cpuapp -T nrf/tests/nrf5340_audio/sw_codec_lc3 -v
