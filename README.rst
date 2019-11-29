
nRF Connect SDK: fw-nrfconnect-nrf 
nRF9160 Sandbox
##################################

This repository contains the Nordic-specific source code additions to open
source projects (Zephyr RTOS and MCUboot).
It must be combined with nrfxlib and the repositories that use the same
naming convention to build the provided samples and to use the additional
subsystems and libraries.

The following repositories must be combined with fw-nrfconnect-nrf:

* fw-nrfconnect-zephyr
* fw-nrfconnect-mcuboot
* nrfxlib

Documentation
*************

Official documentation at:

* Latest: http://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest
* All versions: http://developer.nordicsemi.com/nRF_Connect_SDK/doc/

Disclaimer
**********

nRF Connect SDK supports development with nRF9160 Cellular IoT devices.
It contains references and code for Bluetooth Low Energy devices in the
nRF52 Series, though development on these devices is not currently supported
with the nRF Connect SDK.
