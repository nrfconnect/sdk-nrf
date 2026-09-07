.. nrf70_portable_wifi:

nRF Wi-Fi portable driver
#########################

.. contents::
   :local:
   :depth: 2

The nRF Wi-Fi portable driver implements OS-agnostic code for the nRF70 FullMAC Series of devices.
This code can be used to implement OS-native drivers for the nRF70 Series devices.

The Zephyr native driver implementation is located in the :file:`${ZEPHYR_BASE}/drivers/wifi/nrf_wifi/` folder.

API documentation
*****************

Once the nRF Wi-Fi driver is initialized, the application recognizes it as an Ethernet interface.
To use the Ethernet interface, the application can use the `Zephyr Network APIs`_.
