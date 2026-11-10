.. _cs_de_readme:

Bluetooth Channel Sounding Distance Estimation
##############################################

.. contents::
   :local:
   :depth: 2

This library implements distance estimation algorithms to be used with Bluetooth Channel Sounding and Ranging Profile and Service defined in the `Ranging Service Specification`_ and the `Ranging Profile Specification`_.

Currently, the following methods of computing the distance are available:

* Inverse fourier transform of the mixed signal between initiator and reflector.
  Distance is derived from the time of highest amplitude.
  This amplitude corresponds to the received signal along the shortest path between the devices.
* Derivative of phase with respect to frequency, as described in `Distance estimation based on phase and amplitude information`_.
* Round-trip timing, as described in `Distance estimation based on RTT packets`_.

Configuration
*************

To enable this library, use the :kconfig:option:`CONFIG_BT_CS_DE` Kconfig option.

Check and adjust the following Kconfig options:

* :kconfig:option:`CONFIG_BT_CS_DE_512_NFFT` - Uses 512 samples to compute the inverse fourier transform.
* :kconfig:option:`CONFIG_BT_CS_DE_1024_NFFT` - Uses 1024 samples to compute the inverse fourier transform.
* :kconfig:option:`CONFIG_BT_CS_DE_2048_NFFT` - Uses 2048 samples to compute the inverse fourier transform.

Usage
*****

See :ref:`channel_sounding_ras_initiator`.

Missing tones
=============

The distance estimation functions expect IQ values for all 75 channels, from channel 2 to channel 76.
A channel without a measured tone must be passed as zero.
This is the case for channels 23 to 25, for channels excluded from the channel map, and for tones that the application rejects, for example, due to their tone quality indicator.

A large contiguous block of missing tones acts as a window on the channel response and creates side lobes in the magnitude of the inverse fast Fourier transform (IFFT).
The IFFT method can select such a side lobe as the first path, which gives a too short distance.
This can happen, for example, when a busy Wi-Fi channel lowers the quality of the tones that overlap it.
The phase slope method uses only pairs of adjacent measured tones and is less affected by gaps.

Before you rely on the IFFT estimate, check the number of measured tones and the largest contiguous gap in the procedure.

API documentation
*****************

| Header file: :ncs-file:`/include/bluetooth/cs_de.h`
| Source files: :file:`subsys/bluetooth/cs_de`

.. doxygengroup:: bt_cs_de
