.. _cs_de_readme:

Bluetooth Channel Sounding Distance Estimation
##############################################

.. contents::
   :local:
   :depth: 2

Overview
********

This library implements distance estimation algorithms to be used with Bluetooth Channel Sounding and Ranging Profile/Service defined in the defined in the `Ranging Service Specification`_ and the `Ranging Profile Specification`_.

Currently the following methods of computing the distance are available:

* Fourier transform of the mixed signal between initiator and reflector. Distance is derived from the largest components in the amplitude spectrum.

* Derivative of phase with respect to frequency, as described in `Distance estimation based on phase and amplitude information`_.

* Round-trip timing, as described in `Distance estimation based on RTT packets`_.

Configuration
*************

To enable this library, use the :kconfig:option:`CONFIG_BT_CS_DE` Kconfig option.

Check and adjust the following Kconfig options:

* :kconfig:option:`CONFIG_BT_CS_DE_512_NFFT` - Uses 512 samples to compute the fourier transform.

* :kconfig:option:`CONFIG_BT_CS_DE_1024_NFFT` - Uses 1024 samples to compute the fourier transform.

* :kconfig:option:`CONFIG_BT_CS_DE_2048_NFFT` - Uses 2048 samples to compute the fourier transform.

Usage
*****

| See the sample: :file:`samples/bluetooth/channel_sounding_ras_initiator`

API documentation
*****************

| Header file: :file:`include/bluetooth/cs_de.h`
| Source files: :file:`subsys/bluetooth/cs_de`

.. doxygengroup:: bt_cs_de
