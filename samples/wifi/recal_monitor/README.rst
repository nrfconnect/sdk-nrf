.. _wifi_recal_monitor_sample:

Wi-Fi: Recalibration monitor providers
#######################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates the Wi-Fi recalibration monitor driver's provider
framework: how a channel's live data is supplied, and the two ways to
supply it.
See :ref:`wifi_recal_monitor` for the full driver documentation.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample periodically logs two channels:

* Die temperature — supplied by the driver's built-in, devicetree-selected
  sensor provider (tier 1). On this DK it reads the SoC's own die
  temperature sensor with no devicetree changes.
* Battery voltage — supplied by a mock provider in this sample
  (:file:`src/mock_battery_provider.c`) registered directly with
  :c:macro:`WIFI_RECAL_CHANNEL_DEFINE`, simulating a battery discharging and
  recharging between 3300 and 4200 mV (tier 2). It uses no sensor API at
  all — the point is that a provider's internals can be anything (a
  proprietary fuel-gauge library, a raw register read, and so on) as long
  as it implements the ``init()``/``sample()`` pair.

Configuration
*************

|config|

Devicetree override (tier 1)
=============================

The die-temperature provider normally falls back to the SoC's own ``temp``
sensor. To see the tier-1 override mechanism that lets a board point the
same provider at a different sensor, build with the extra overlay in this
sample directory:

.. code-block:: console

   west build -p -b nrf7120dk/nrf7120/cpuapp samples/wifi/recal_monitor -- \
      -DEXTRA_DTC_OVERLAY_FILE=chosen_override.overlay

This DK has no second temperature sensor, so the overlay points the
``nordic,wifi-recal-die-temp-sensor`` chosen property back at the same ``temp``
node — it exists to prove the override is picked up, not to change the
reading. On a custom board, point it at your own sensor's nodelabel
instead.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/recal_monitor`

.. include:: /includes/build_and_run_ns.txt

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

      [00:00:02.011,000] <inf> wifi_recal_sample: die temp: 24.87 C (tier 1, built-in sensor provider) | battery: 4180 mV (tier 2, custom mock provider)
      [00:00:04.011,000] <inf> wifi_recal_sample: die temp: 24.93 C (tier 1, built-in sensor provider) | battery: 4160 mV (tier 2, custom mock provider)
