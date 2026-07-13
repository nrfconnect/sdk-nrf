.. _wifi_recal_monitor_sample:

Wi-Fi: Recalibration monitor providers
######################################

.. contents::
   :local:
   :depth: 2

The Recalibration monitor providers sample demonstrates the :ref:`platform metrics library's <platform_metrics>` provider framework for a Wi-Fi recalibration use case, showing how a channel's live data is supplied and the two ways to supply it.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

The sample also requires the ``nordic,wifi-xo-freq-offset`` devicetree property on the Wi-Fi node, added to the ``nordic,nrf7120-wifi`` binding by `sdk-zephyr PR #4260`_.

Overview
********

The sample periodically logs two ``platform_metrics`` channels, along with the board's XO frequency offset:

* Die temperature - It is supplied by the library's built-in, devicetree-selected sensor provider (tier 1).
  On this DK, it reads the SoC's internal die temperature sensor without any devicetree changes.
* Battery voltage - It is supplied by a mock provider in this sample (:file:`src/mock_battery_provider.c`), which is registered directly with :c:macro:`PLATFORM_METRICS_CHANNEL_DEFINE`, and simulates a battery discharging and recharging between 3300 mV and 4200 mV (tier 2).
  It does not use the sensor API.
  This demonstrates that a provider's implementation can be anything, such as a proprietary fuel-gauge library or a raw register read, as long as it implements the ``init()`` and ``sample()`` functions.
* XO frequency offset - It is not a ``platform_metrics`` channel, because it is a fixed per-board-design constant rather than a value that drifts at runtime.
  The sample reads it directly from the ``nordic,wifi-xo-freq-offset`` devicetree property on the Wi-Fi® node, just as the Wi-Fi driver does.

Configuration
*************

|config|

Devicetree override (tier 1)
============================

The die temperature provider normally falls back to the SoC's internal ``temp`` sensor.
To see the tier-1 override mechanism that allows a board to point the same provider to a different sensor, build with the additional overlay in this sample directory:

.. code-block:: console

   west build -p -b nrf7120dk/nrf7120/cpuapp samples/wifi/recal_monitor -- \
      -DEXTRA_DTC_OVERLAY_FILE=chosen_override.overlay

This DK does not include a second temperature sensor.
The overlay therefore points the ``nordic,platform-metrics-die-temp-sensor`` chosen property to the same ``temp`` node.

This configuration demonstrates that the override is applied.
It does not change the temperature reading.

On a custom board, point this property to the nodelabel of your own sensor.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/recal_monitor`

.. include:: /includes/build_and_run_ns.txt

To build for the nRF7120 DK, use the ``nrf7120dk/nrf7120/cpuapp`` board target.
The following is an example of the CLI command:

.. code-block:: console

   west build -p -b nrf7120dk/nrf7120/cpuapp samples/wifi/recal_monitor

.. include:: /includes/wifi_refer_sample_yaml_file.txt

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Observe the periodic log output, for example:

   .. code-block:: console

      [00:00:02.011,000] <inf> wifi_recal_sample: die temp: 24.87 C (tier 1, built-in sensor provider) | battery: 4180 mV (tier 2, custom mock provider) | XO offset: 0 ppm (board devicetree constant)
      [00:00:04.011,000] <inf> wifi_recal_sample: die temp: 24.93 C (tier 1, built-in sensor provider) | battery: 4160 mV (tier 2, custom mock provider) | XO offset: 0 ppm (board devicetree constant)

Dependencies
************

This sample uses the following |NCS| library:

* :ref:`platform_metrics`
