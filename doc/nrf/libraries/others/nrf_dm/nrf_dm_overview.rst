.. _nrf_dm_overview:

Overview
########

.. contents::
   :local:
   :depth: 2

The library can use different methods to measure the distance between devices:

   * Round-trip timing
   * Multi-carrier phase difference:

      * IFFT of spectrum
      * Average phase-slope estimation
      * Friis-path-loss formula
   * Link loss

It does also provide the following features:

* Quality indicator
* TX power and RSSI for both the remote and the local devices

Round-trip timing
*****************

When using round-trip timing (RTT), the library measures the distance between nodes based on the transmission delay of the wave between them.
Knowing the time it takes to transmit a message, the library can estimate the distance between the nodes.

Multi-carrier phase difference
******************************

When using multi-carrier phase ranging, the library measures the phase difference in RF signals between two devices as a function of the frequency.
See the following example:

1. The initiator transmits a signal to the reflector.
#. The reflector measures the phase difference to its local reference.
#. The reflector transmits a signal back to the initiator on the same frequency.
#. The initiator measures the phase difference to its local reference.
#. The initiator and reflector jump to a new frequency and repeat the measurement.
#. The library then calculates the distance between devices based on the measured phase differences as a function of the frequency.
