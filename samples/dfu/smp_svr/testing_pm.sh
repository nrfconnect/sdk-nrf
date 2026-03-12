#!/usr/bin/bash

# Build a configuration with NSIB and partition manager
west build -p -b nrf54lv10dk/nrf54lv10a/cpuapp -T ./sample.dfu.smp_svr.nsib.kmu.lcs
# Program the main app, MCUboot, NSIB and provision both the manufacturing and the main app public key
west flash --erase
# Overwrtie the main app with the manufacturing application
nrfutil device program --firmware ./build/manufacturing_app/zephyr/zephyr.signed.hex --options chip_erase_mode=ERASE_RANGES_TOUCHED_BY_FIRMWARE
# Create a confirmed (permanent) update candidate HEX file for direct programming
bin2hex.py --offset 0x8b000 ./build/smp_svr/zephyr/zephyr.signed.confirmed.bin update.hex
# Provide a main application as a confirmed (permanent) update candidate
nrfutil device program --firmware ./update.hex --options chip_erase_mode=ERASE_RANGES_TOUCHED_BY_FIRMWARE

