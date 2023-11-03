.. _nrf_802154_sl:

nRF 802.15.4 Service Layer
==========================

The nRF 802.15.4 Service Layer (SL) is a library that implements the following features of the nRF 802.15.4 Radio Driver:

* Multiprotocol support.
  It allows the concurrency of Bluetooth LE and 802.15.4 operations.
* CSMA/CA support.
  It provides a built-in CSMA/CA mechanism, optimized for performance.
* Delayed transmission and reception.
  It allows the scheduling of transmissions and receptions, to execute them at a predefined moment.
* Timestamping.
  It provides precise frame timestamps.

It also provides an API to retrieve the capabilities of the binary in runtime.

The service layer is available as a binary for the following SoCs:

* nRF52833
* nRF52840
* nRF5340 Network Core

For the SOCs equipped with a floating-point unit (nRF52840 and nRF52833), the service layer is available in the soft-float, softfp-float, and hard-float build versions.
For the other SOCs/SiPs, it is available only in the soft-float build version.

When using the |NCS|, the nRF 802.15.4 Radio Driver takes advantage of the service layer automatically.
No additional user action is needed.
Instead, when using the nRF 802.15.4 Radio Driver in your project outside of the |NCS|, you must do the following to let the radio driver take advantage of the features enabled by the service layer:

* Add the header files to the include directories of the project.
* Add the binaries to the linking stage of the build.
