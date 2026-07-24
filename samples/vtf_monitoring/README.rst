.. _vtf_monitoring_sample:

VTF monitoring
##############

.. contents::
   :local:
   :depth: 2

The VTF monitoring sample demonstrates how voltage-temperature-frequency (VTF) data is captured and stored for the nRF Wi-Fi® subsystem.
The sample monitors battery voltage, die temperature, and crystal oscillator (XO) frequency offset.
The Wi-Fi core uses this data to determine when recalibration is required.
The :ref:`vtf_monitoring` subsystem updates the snapshots on the application core and stores them in a dedicated memory region that the Wi-Fi core can read.
This sample runs on the application core and periodically logs the stored channel values.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

The sample periodically logs the following :ref:`vtf_monitoring` channels:

* Die temperature - Supplied by the subsystem built-in provider, which reads the SoC internal die temperature sensor.
* Battery voltage - In the default configuration, the :kconfig:option:`CONFIG_VTF_BATTERY_VOLTAGE_MONITOR` Kconfig option is enabled.
  Live capture for battery voltage is not yet implemented in the subsystem, so the snapshot holds the default value of the Kconfig option.
* Crystal oscillator (XO) frequency offset - Since the :kconfig:option:`CONFIG_VTF_FREQ_OFFSET_MONITOR` Kconfig option is disabled, the snapshot holds the default value of the Kconfig option.

The sample includes a mock provider in the :file:`src/mock_battery_provider.c` file to demonstrate a custom channel monitoring backend.
It registers a channel with :c:macro:`VTF_CHANNEL_DEFINE` and simulates a battery discharging from 4200 mV to 3300 mV.
This shows that a provider can use any data source, such as a fuel gauge library or a register read, as long as it implements the ``init()`` and ``sample()`` functions described in the subsystem documentation.

Configuration
*************

|config|

Devicetree configuration
========================

The board overlay in the :file:`boards/nrf7120dk_nrf7120_cpuapp.overlay` file configures the following:

* ``nordic,vtf-region`` - Selects the SRAM region where ``vtf_snapshots`` are stored.

.. _vtf_monitoring_sample_custom_battery:

Custom battery voltage backend
==============================

To build the variant that uses the mock battery voltage provider instead of the default backend, disable :kconfig:option:`CONFIG_VTF_BATTERY_VOLTAGE_MONITOR` and enable :kconfig:option:`CONFIG_SAMPLE_VTF_CUSTOM_BATTERY_VOLTAGE_MONITOR`:

.. code-block:: console

   west build -p -b nrf7120dk/nrf7120/cpuapp samples/vtf_monitoring -- -DCONFIG_VTF_BATTERY_VOLTAGE_MONITOR=n -DCONFIG_SAMPLE_VTF_CUSTOM_BATTERY_VOLTAGE_MONITOR=y

In this configuration, the sample logs a decreasing battery voltage until it resets to 4200 mV.

Building and running
********************

.. |sample path| replace:: :file:`samples/vtf_monitoring`

To build for the nRF7120 DK, use the ``nrf7120dk/nrf7120/cpuapp`` board target.
The following is an example of the CLI command:

.. code-block:: console

   west build -p -b nrf7120dk/nrf7120/cpuapp samples/vtf_monitoring

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Observe the periodic log output.
   With the default configuration, the output looks similar to the following example:

   .. code-block:: console

      [00:00:00.013,670] <inf> vtf_monitoring_sample: die temp: 25 C
      [00:00:00.013,680] <inf> vtf_monitoring_sample: battery voltage: 3500 mV
      [00:00:00.013,688] <inf> vtf_monitoring_sample: XO offset: 0 ppm

   Die temperature reflects live readings from the die temperature sensor.
   Battery voltage and XO offset show the configured default values.

#. Build and run the :ref:`custom battery voltage backend <vtf_monitoring_sample_custom_battery>` variant and observe that the battery voltage decreases over time.

Dependencies
************

This sample uses the following |NCS| subsystem:

* :ref:`vtf_monitoring`
