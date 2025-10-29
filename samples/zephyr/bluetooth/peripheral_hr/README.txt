This sample extends the same-named Zephyr sample to verify it with Nordic nRF54lv10 development kit.

Source code and basic configuration files can be found in the corresponding folder structure in zephyr/samples/bluetooth/peripheral_hr.

Building
********

To build the sample for the nRF54LV10 DK, use the following command:

   west build -p -b nrf54lv10dk/nrf54lv10a/cpuapp -d build_54lv10 -T nrf.extended.sample.bluetooth.peripheral_hr .
