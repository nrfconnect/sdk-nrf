nRF Connect SDK: sdk-nrf
########################

.. contents::
   :local:
   :depth: 2

This repository contains the Nordic-specific source code additions to open
source projects (Zephyr RTOS and MCUboot).
It must be combined with nrfxlib and the repositories that use the same
naming convention to build the provided samples and to use the additional
subsystems and libraries.

The following repositories must be combined with sdk-nrf:

* sdk-zephyr
* sdk-mcuboot
* nrfxlib

Documentation
*************

Official documentation at:

* Latest: http://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest
* All versions: http://developer.nordicsemi.com/nRF_Connect_SDK/doc/

Disclaimer
**********

nRF Connect SDK supports development with nRF9160 Cellular IoT devices.
nRF53 Series devices (which are pre-production) and Zigbee and Bluetooth mesh protocols are supported for development in v1.4.2 for prototyping and evaluation.
Support for production and deployment in end products is coming soon.
