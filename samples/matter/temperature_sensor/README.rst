.. |matter_name| replace:: Temperature Sensor
.. |matter_type| replace:: sample
.. |matter_dks_thread| replace:: ``nrf52840dk/nrf52840``, ``nrf5340dk/nrf5340/cpuapp``, ``nrf54l15dk/nrf54l15/cpuapp``, and ``nrf54lm20dk/nrf54lm20a/cpuapp`` board targets
.. |matter_dks_internal| replace:: nRF54LM20 DK
.. |sample path| replace:: :file:`samples/matter/temperature_sensor`
.. |matter_qr_code_payload| replace:: MT:K.K9042C00KA0648G00
.. |matter_pairing_code| replace:: 34970112332
.. |matter_qr_code_image| image:: /images/matter_qr_code_temperature_sensor.png

.. include:: /includes/matter/shortcuts.txt

.. _matter_temperature_sensor_sample:

Matter: Temperature Sensor
##########################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use the :ref:`Matter <ug_matter>` application layer to build a device capable of measuring temperature.

.. include:: /includes/matter/introduction/sleep_thread_sed_lit.txt

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/matter/requirements/thread.txt

Overview
********

The sample does not use a real temperature sensor due to hardware limitation.
Instead, it simulates temperature measurement following the linearly increasing values from –20 to +20 Celsius degrees.
The measurement results are updated every 10 s and after reaching the maximum value, the temperature drops to the minimum and starts to increase from the beginning.

You can test the device remotely over a Thread network, which requires more devices.

The remote control testing requires a Matter controller that you can configure either on a PC or a mobile device.
You can enable both methods after `Commissioning the device`_.

.. include:: /includes/matter/overview/matter_quick_start.txt

Temperature Sensor features
===========================

This sample implements the following features:

* :ref:`ICD LIT <matter_temperature_sensor_sample_lit>` - The temperature sensor can be used as an Intermittently Connected Device (ICD) with a :ref:`Long Idle Time (LIT)<ug_matter_device_low_power_icd_sit_lit>`.

Use the ``click to show`` toggle to expand the content.

.. _matter_temperature_sensor_sample_lit:

.. include:: /includes/matter/overview/icd_lit.txt

Configuration
*************

.. include:: /includes/matter/configuration/intro.txt

The |matter_type| supports the following build configurations:

.. include:: /includes/matter/configuration/basic_internal.txt

Advanced configuration options
==============================

.. include:: /includes/matter/configuration/advanced/intro.txt
.. include:: /includes/matter/configuration/advanced/dfu.txt
.. include:: /includes/matter/configuration/advanced/factory_data.txt
.. include:: /includes/matter/configuration/advanced/custom_board.txt
.. include:: /includes/matter/configuration/advanced/internal_memory.txt

User interface
**************

.. include:: /includes/matter/interface/intro.txt

First LED:
   .. include:: /includes/matter/interface/state_led.txt

Second LED:
   The LED starts blinking evenly (500 ms on/500 ms off) when the ``Identify`` command of the Identify cluster is received on the endpoint ``1``.
   You can use the command's argument to specify the duration of the effect.

First Button:
   .. include:: /includes/matter/interface/main_button.txt

Third Button:
   Functions as the User Active Mode Trigger (UAT) button.
   For more information about Intermittently Connected Devices (ICD) and User Active Mode Trigger, see the :ref:`ug_matter_device_low_power_icd` documentation section.

.. include:: /includes/matter/interface/segger_usb.txt
.. include:: /includes/matter/interface/nfc.txt

Building and running
********************

.. include:: /includes/matter/building_and_running/intro.txt
.. include:: /includes/matter/building_and_running/select_configuration.txt
.. include:: /includes/matter/building_and_running/commissioning.txt

|matter_ble_advertising_auto|

.. include:: /includes/matter/building_and_running/onboarding.txt

Testing
*******

.. |endpoint_name| replace:: **Temperature Measurement cluster**
.. |endpoint_id| replace:: **1**

.. include:: /includes/matter/testing/intro.txt
.. include:: /includes/matter/testing/prerequisites.txt
.. include:: /includes/matter/testing/prepare.txt

Testing steps
=============

#. Read the temperature measured by the device by invoking the following command:

   .. parsed-literal::
      :class: highlight

      ./chip-tool temperaturemeasurement read measured-value <node_id> 1

   The received output will look similar to the following:

   .. code-block:: console

      [1755081048.320] [99348:99350] [TOO] Endpoint: 1 Cluster: 0x0000_0402 Attribute 0x0000_0000 DataVersion: 1994139940
      [1755081048.320] [99348:99350] [TOO]   MeasuredValue: 9

#. Wait some time, for example 30 s, and read the measured temperature again using the same command as before:

   .. parsed-literal::
      :class: highlight

      ./chip-tool temperaturemeasurement read measured-value <node_id> 1

   The received value will be different, for example:

   .. code-block:: console

      [1755081048.320] [99348:99350] [TOO] Endpoint: 1 Cluster: 0x0000_0402 Attribute 0x0000_0000 DataVersion: 1994139940
      [1755081048.320] [99348:99350] [TOO]   MeasuredValue: 1200

Factory reset
=============

|matter_factory_reset|

Dependencies
************

.. include:: /includes/matter/dependencies.txt
