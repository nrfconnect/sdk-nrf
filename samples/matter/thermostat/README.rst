.. |matter_name| replace:: Thermostat
.. |matter_type| replace:: sample
.. |matter_dks_thread| replace:: ``nrf52840dk/nrf52840``, ``nrf5340dk/nrf5340/cpuapp``, ``nrf54l15dk/nrf54l15/cpuapp``, and ``nrf54lm20dk/nrf54lm20a/cpuapp`` board targets
.. |matter_dks_wifi| replace:: ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target with the ``nrf7002eb2`` shield attached
.. |matter_dks_internal| replace:: nRF54LM20 DK
.. |matter_dks_tfm| replace:: ``nrf54l15dk/nrf54l15/cpuapp`` board target
.. |sample path| replace:: :file:`samples/matter/thermostat`
.. |matter_qr_code_payload| replace:: MT:O4CT342C00KA0648G00
.. |matter_pairing_code| replace:: 34970112332
.. |matter_qr_code_image| image:: /images/matter_qr_code_thermostat.png

.. include:: /includes/matter/shortcuts.txt

.. _matter_thermostat_sample:

Matter: Thermostat
##################

.. contents::
   :local:
   :depth: 2

This thermostat sample demonstrates the usage of the :ref:`Matter <ug_matter>` application layer to build a thermostat device for monitoring temperature values and controlling the temperature.
This device works as a Matter accessory device, meaning it can be paired and controlled remotely over a Matter network built on top of a low-power, 802.15.4 Thread network or on top of a Wi-Fi® network.

.. include:: /includes/matter/introduction/sleep_thread_wifi.txt

Additionally, this example allows you to connect to a temperature sensor device that can also be used for temperature measurement.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/matter/requirements/thread_wifi.txt

Similarly, if you want to test the sample with :ref:`matter_thermostat_sample_sensor`, you need additional hardware that incorporates a temperature sensor.
For example, Nordic Thingy:53, used for the :ref:`Matter weather station <matter_weather_station_app>` application.

Overview
********

The sample can operate in one of the following modes:

* Simulated temperature sensor mode - In this mode, the thermostat sample generates simulated temperature measurements and prints it to the terminal.
  This is the default mode, in which the sample provides simulated temperature values.
* Real temperature sensor mode - In this mode, the thermostat sample is bound to a remote Matter temperature sensor, which provides real temperature measurements.
  This mode requires :ref:`matter_thermostat_sample_sensor`.

  .. figure:: ../../../doc/nrf/images/matter_external_thermostat_setup.png
     :alt: Real temperature sensor mode setup

     Real temperature sensor mode setup

The sample automatically logs the temperature measurements with a defined interval and it uses buttons for printing the measurement results to the terminal.

You can test the sample in the following ways:

* Standalone, using a single DK that runs the thermostat application.
* Remotely over the Thread or the Wi-Fi protocol, which in either case requires more devices, including a Matter controller that you can configure either on a PC or a mobile device.

Thermostat features
===================

The thermostat sample implements the following features:

* Temperature measurement - The thermostat sample can measure the temperature and print it to the terminal.
* Temperature control - The thermostat sample can control the temperature of the connected devices.
* :ref:`Binding <matter_thermostat_sample_binding>` - The thermostat sample can bind with the temperature sensor to be able to control it.

.. _matter_thermostat_sample_binding:

.. include:: /includes/matter/overview/binding.txt

Configuration
*************

.. include:: /includes/matter/configuration/intro.txt
The |matter_type| supports the following build configurations:

.. include:: /includes/matter/configuration/basic_internal.txt

Advanced configuration options
==============================

.. include:: /includes/matter/configuration/advanced/intro.txt

.. _matter_thermostat_sample_sensor:

External sensor integration
---------------------------

.. toggle::

   The thermostat sample lets you connect to an external temperature sensor, for example :ref:`Matter weather station application on Nordic Thingy:53 <matter_weather_station_app>`.
   This establishes the :ref:`matter_thermostat_sample_binding` to Matter's temperature measurement cluster.

   By default, the thermostat sample generates simulated temperature measurements that simulate local temperature changes.
   Additionally, you can enable periodic outdoor temperature measurements by binding the thermostat with an external temperature sensor device.
   To test this feature, follow the steps listed in the :ref:`matter_thermostat_sensor_testing` section.

.. include:: /includes/matter/configuration/advanced/dfu.txt
.. include:: /includes/matter/configuration/advanced/tfm.txt
.. include:: /includes/matter/configuration/advanced/factory_data.txt
.. include:: /includes/matter/configuration/advanced/custom_board.txt
.. include:: /includes/matter/configuration/advanced/internal_memory.txt

User interface
**************

.. include:: /includes/matter/interface/intro.txt

First LED:
   .. include:: /includes/matter/interface/state_led.txt

First Button:
   .. include:: /includes/matter/interface/main_button.txt

Second Button:
   Prints the most recent thermostat data to terminal.

.. include:: /includes/matter/interface/segger_usb.txt
.. include:: /includes/matter/interface/nfc.txt

Building and running
********************

.. include:: /includes/matter/building_and_running/intro.txt

|matter_ble_advertising_auto|

Advanced building options
=========================

.. include:: /includes/matter/building_and_running/advanced/building_nrf54lm20dk_7002eb2.txt
.. include:: /includes/matter/building_and_running/advanced/wifi_flash.txt

Testing
*******

.. include:: /includes/matter/testing/intro.txt

.. _matter_thermostat_sample_testing_start:

Testing with CHIP Tool
======================

Complete the following steps to test the |matter_name| device using CHIP Tool:

.. |node_id| replace:: 1

.. include:: /includes/matter/testing/1_prepare_matter_network_thread_wifi.txt
.. include:: /includes/matter/testing/2_prepare_dk.txt
.. include:: /includes/matter/testing/3_commission_thread_wifi.txt

.. rst-class:: numbered-step

Read the simulated temperature
------------------------------

Read the simulated temperature by running the following command:

.. parsed-literal::
   :class: highlight

   ./chip-tool temperaturemeasurement read measured-value <node_id> 1

The received output will look similar to the following:

.. code-block:: console

   [1755081048.320] [99348:99350] [TOO] Endpoint: 1 Cluster: 0x0000_0402 Attribute 0x0000_0000 DataVersion: 1994139940
   [1755081048.320] [99348:99350] [TOO]   MeasuredValue: 9

.. _matter_thermostat_sensor_testing:

Testing with external sensor
============================

Complete the following steps to test communication between the thermostat and the temperature sensor:

.. rst-class:: numbered-step

Prepare both devices
--------------------

Follow the steps in the :ref:`matter_thermostat_sample_testing_start` section
During the commissioning process, write down the values for the thermostat node ID, the temperature sensor node ID, and the temperature sensor endpoint ID.
These IDs are going to be used in the next steps (*<thermostat_node_ID>*, *<temperature_sensor_node_ID>*, and *<temperature_sensor_endpoint_ID>*, respectively).

.. rst-class:: numbered-step

Add proper ACL for the temperature sensor device
-------------------------------------------------

Use the :doc:`CHIP Tool <matter:chip_tool_guide>` ("Writing ACL to the ``accesscontrol`` cluster" section) to add proper ACL for the temperature sensor device.
Use the following command, with *<thermostat_node_ID>*, *<temperature_sensor_node_ID>*, and *<temperature_sensor_endpoint_ID>* values from the previous step about commissioning:

.. parsed-literal::
   :class: highlight

   chip-tool accesscontrol write acl '[{"fabricIndex": 1, "privilege": 5, "authMode": 2, "subjects": [112233], "targets": null}, {"fabricIndex": 1, "privilege": 1, "authMode": 2, "subjects": [<thermostat_node_ID>], "targets": [{"cluster": 1026, "endpoint": <temperature_sensor_endpoint_ID>, "deviceType": null}]}]' <temperature_sensor_node_ID> 0

.. rst-class:: numbered-step

Write a binding table
---------------------

Write a binding table to the thermostat to inform the device about the temperature sensor endpoint by running the following command, with *<thermostat_node_ID>*, *<temperature_sensor_node_ID>*, and *<temperature_sensor_endpoint_ID>* values from the previous step about commissioning:

.. parsed-literal::
   :class: highlight

   chip-tool binding write binding '[{"fabricIndex": 1, "node": <temperature_sensor_node_ID>, "endpoint": <temperature_sensor_endpoint_ID>, "cluster": 1026}]' <thermostat_node_ID> 1

(You can read more about this step in the "Adding a binding table to the ``binding`` cluster" in the :doc:`CHIP Tool <matter:chip_tool_guide>` guide.)

The thermostat is now able to read the real temperature data from the temperature sensor device.
The connection is ensured by :ref:`matter_thermostat_sample_binding` to Matter's temperature measurement cluster.

.. rst-class:: numbered-step

Press the |Second Button|
-------------------------

Press the |Second Button| to print the most recent temperature data from the thermostat device to the UART terminal.

Testing with commercial ecosystem
=================================

.. include:: /includes/matter/testing/ecosystem.txt

Dependencies
************
.. include:: /includes/matter/dependencies.txt
