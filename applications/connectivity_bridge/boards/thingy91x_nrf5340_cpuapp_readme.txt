=====================
  Nordic Thingy:91 X
=====================

This USB interface has the following functions:
* Disk drive containing this file and others
* COM ports for nRF91 debug, trace, and firmware update

COM Ports
====================
This USB interface exposes two COM ports mapped to the physical UART interfaces between the nRF91 Series and nRF5340 devices.
When opening these ports manually (without using the LTE Link Monitor), be aware that the USB COM port baud rate selection is applied to the UART.

Bluetooth® LE Central UART Service
==================================

This device advertises as "Thingy:91 X UART".
Connect using a Bluetooth LE Central device, for example a phone running the nRF Connect app:
https://www.nordicsemi.com/Software-and-tools/Development-Tools/nRF-Connect-for-mobile/

NOTE: The Bluetooth LE interface is unencrypted and intended to be used during debugging.
      By default, the Bluetooth LE interface is disabled.
      Enable it by setting the appropriate option in Config.txt.
