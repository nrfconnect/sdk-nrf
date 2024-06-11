.. _matter_thermostat_sample:

Matter: Thermostat
##################

.. contents::
   :local:
   :depth: 2

This thermostat sample demonstrates the usage of the :ref:`Matter <ug_matter>` application layer to build a thermostat device for monitoring temperature values and controlling the temperature.
This device works as a Matter accessory device, meaning it can be paired and controlled remotely over a Matter network built on top of a low-power, 802.15.4 Thread network or on top of a Wi-Fi network.
In case of Thread, this device works as a Thread :ref:`Minimal End Device <thread_ot_device_types>`.
Support for both Thread and Wi-Fi is mutually exclusive and depends on the hardware platform, so only one protocol can be supported for a specific Matter device.

Additionally, this example allows you to connect to a temperature sensor device that can also be used for temperature measurement.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

If you want to commission the device and :ref:`control it remotely <matter_thermostat_network_mode>` in a Thread network, you also need a Matter controller device :ref:`configured on PC or smartphone <ug_matter_configuring>`.
This requires additional hardware depending on the setup you choose.

Similarly, if you want to test the sample with :ref:`matter_thermostat_sample_sensor`, you need additional hardware that incorporates a temperature sensor.
For example, Nordic Thingy:53, used for the :ref:`Matter weather station <matter_weather_station_app>` application.

.. note::
    |matter_gn_required_note|

IPv6 network support
====================

The development kits for this sample offer the following IPv6 network support for Matter:

* Matter over Thread is supported for ``nrf52840dk/nrf52840``, ``nrf5340dk/nrf5340/cpuapp``, and ``nrf54l15pdk/nrf54l15/cpuapp``.
* Matter over Wi-Fi is supported for ``nrf5340dk/nrf5340/cpuapp`` with the ``nrf7002_ek`` shield attached or for ``nrf7002dk/nrf5340/cpuapp``.

Overview
********

When programmed, the sample starts the Bluetooth® LE advertising automatically and prepares the Matter device for commissioning into a Matter-enabled Thread network.
The sample uses an LED to show the state of the connection.

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

You can enable both methods after :ref:`building and running the sample <matter_thermostat_sample_remote_control>`.

Configuration
*************

|config|

Access Control List
===================

The Access Control List (ACL) is a list related to the Access Control cluster.
The list contains rules for managing and enforcing access control for a node's endpoints and their associated cluster instances.
In this sample's case, this allows the temperature measurement devices to receive messages from the thermostat and provide the temperature data to the thermostat.

You can read more about ACLs on the :doc:`matter:access-control-guide` in the Matter documentation.

.. _matter_thermostat_sample_sensor:

External sensor integration
===========================

The thermostat sample lets you connect to an external temperature sensor, for example :ref:`Matter weather station application on Nordic Thingy:53 <matter_weather_station_app>`.
This establishes the :ref:`matter_thermostat_sample_binding` to Matter's temperature measurement cluster.

By default, the thermostat sample generates simulated temperature measurements that simulate local temperature changes.
Additionally, you can enable periodic outdoor temperature measurements by binding the thermostat with an external temperature sensor device.
To test this feature, follow the steps listed in the :ref:`matter_thermostat_sensor_testing` section.

.. _matter_thermostat_sample_binding:

Binding
=======

.. include:: ../light_switch/README.rst
   :start-after: matter_light_switch_sample_binding_start
   :end-before: matter_light_switch_sample_binding_end

In this sample, the thermostat device prints simulated temperature data by default and it does not know the remote endpoints of the temperature sensors (on remote nodes).
Using binding, the thermostat device updates its Binding cluster with all relevant information about the temperature sensor devices, such as their IPv6 address, node ID, and the IDs of the remote endpoints that contain the temperature measurement cluster.

.. _matter_thermostat_custom_configs:

Matter thermostat custom configurations
=======================================

.. include:: ../light_bulb/README.rst
    :start-after: matter_light_bulb_sample_configuration_file_types_start
    :end-before: matter_light_bulb_sample_configuration_file_types_end

Device Firmware Upgrade support
===============================

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_build_with_dfu_start
    :end-before: matter_door_lock_sample_build_with_dfu_end

.. _matter_thermostat_network_mode:

Remote testing in a network
===========================

.. include:: ../light_bulb/README.rst
    :start-after: matter_light_bulb_sample_remote_testing_start
    :end-before: matter_light_bulb_sample_remote_testing_end

Factory data support
====================

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_factory_data_start
    :end-before: matter_door_lock_sample_factory_data_end

User interface
**************

.. tabs::

   .. group-tab:: nRF52, nRF53 and nRF70 DKs

      LED 1:
         .. include:: /includes/matter_sample_state_led.txt

      Button 1:
         .. include:: /includes/matter_sample_button.txt

      Button 2:
         Prints the most recent thermostat data to terminal.

      .. include:: /includes/matter_segger_usb.txt

      NFC port with antenna attached:
          Optionally used for obtaining the `Onboarding information`_ from the Matter accessory device to start the :ref:`commissioning procedure <matter_thermostat_sample_remote_control>`.

   .. group-tab:: nRF54 DKs

      LED 0:
         .. include:: /includes/matter_sample_state_led.txt

      Button 0:
         .. include:: /includes/matter_sample_button.txt

      Button 1:
         Prints the most recent thermostat data to terminal.

      .. include:: /includes/matter_segger_usb.txt

      NFC port with antenna attached:
          Optionally used for obtaining the `Onboarding information`_ from the Matter accessory device to start the :ref:`commissioning procedure <matter_thermostat_sample_remote_control>`.

Building and running
********************

.. |sample path| replace:: :file:`samples/matter/thermostat`

.. include:: /includes/build_and_run.txt

Selecting a build type
======================

Before you start testing the application, you can select one of the :ref:`matter_thermostat_custom_configs`, depending on your building method.
See :ref:`app_build_file_suffixes` and :ref:`cmake_options` for more information how to select a configuration.

.. _matter_thermostat_testing:

Testing
=======

After building the sample and programming it to your development kit, you can either test the sample's basic features or use the Matter weather station application to :ref:`test the thermostat with an external sensor <matter_thermostat_sensor_testing>`.

.. _matter_thermostat_sample_basic_features_tests:

Testing basic features
----------------------

After building the sample and programming it to your development kit, complete the following steps to test its basic features:

.. tabs::

   .. group-tab:: nRF52, nRF53 and nRF70 DKs

      1. |connect_kit|
      #. |connect_terminal_ANSI|
      #. Observe that **LED 1** starts flashing (short flash on).
         This means that the sample has automatically started the Bluetooth LE advertising.
      #. Observe the UART terminal.
         The sample starts automatically printing the simulated temperature data to the terminal with 30-second intervals.
      #. Press **Button 2** to print the most recent temperature data to the terminal.
      #. Keep **Button 1** pressed for more than six seconds to initiate factory reset of the device.

         The device reboots after all its settings are erased.

   .. group-tab:: nRF52, nRF53 and nRF70 DKs

      1. |connect_kit|
      #. |connect_terminal_ANSI|
      #. Observe that **LED 0** starts flashing (short flash on).
         This means that the sample has automatically started the Bluetooth LE advertising.
      #. Observe the UART terminal.
         The sample starts automatically printing the simulated temperature data to the terminal with 30-second intervals.
      #. Press **Button 1** to print the most recent temperature data to the terminal.
      #. Keep **Button 0** pressed for more than six seconds to initiate factory reset of the device.

         The device reboots after all its settings are erased.

.. _matter_thermostat_sensor_testing:

Testing with external sensor
----------------------------

After building this sample and the :ref:`Matter weather station <matter_weather_station_app>` application and programming each to the respective development kit and Nordic Thingy:53, complete the following steps to test communication between both devices:

1. |connect_terminal_both|
#. |connect_terminal_both_ANSI|
#. If devices were not erased during the programming, press the button responsible for the factory reset on each device.
#. On each device, press the button that starts the Bluetooth LE advertising.
#. Commission devices to the Matter network.
   See `Commissioning the device`_ for more information.

   During the commissioning process, write down the values for the thermostat node ID, the temperature sensor node ID, and the temperature sensor endpoint ID.
   These IDs are going to be used in the next steps (*<thermostat_node_ID>*, *<temperature_sensor_node_ID>*, and *<temperature_sensor_endpoint_ID>*, respectively).
#. Use the :doc:`CHIP Tool <matter:chip_tool_guide>` ("Writing ACL to the ``accesscontrol`` cluster" section) to add proper ACL for the temperature sensor device.
   Use the following command, with *<thermostat_node_ID>*, *<temperature_sensor_node_ID>*, and *<temperature_sensor_endpoint_ID>* values from the previous step about commissioning:

   .. code-block:: console

      chip-tool accesscontrol write acl '[{"fabricIndex": 1, "privilege": 5, "authMode": 2, "subjects": [112233], "targets": null}, {"fabricIndex": 1, "privilege": 1, "authMode": 2, "subjects": [<thermostat_node_ID>], "targets": [{"cluster": 1026, "endpoint": <temperature_sensor_endpoint_ID>, "deviceType": null}]}]' <temperature_sensor_node_ID> 0

#. Write a binding table to the thermostat to inform the device about the temperature sensor endpoint.
   Use the following command, with *<thermostat_node_ID>*, *<temperature_sensor_node_ID>*, and *<temperature_sensor_endpoint_ID>* values from the previous step about commissioning:

   .. code-block:: console

      chip-tool binding write binding '[{"fabricIndex": 1, "node": <temperature_sensor_node_ID>, "endpoint": <temperature_sensor_endpoint_ID>, "cluster": 1026}]' <thermostat_node_ID> 1

   (You can read more about this step in the "Adding a binding table to the ``binding`` cluster" in the CHIP Tool guide.)

   The thermostat is now able to read the real temperature data from the temperature sensor device.
   The connection is ensured by :ref:`matter_thermostat_sample_binding` to Matter's temperature measurement cluster.

#. Press the button that prints the most recent temperature data from the thermostat device to the UART terminal.

.. _matter_thermostat_sample_remote_control:

Enabling remote control
=======================

Remote control allows you to control the Matter thermostat device from an IPv6 network.

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_remote_control_start
    :end-before: matter_door_lock_sample_remote_control_end

.. _matter_thermostat_sample_remote_control_commissioning:

Commissioning the device
------------------------

.. note::
   Before starting the commissioning to Matter procedure, ensure that there is no other Bluetooth LE connection established with the device.

.. include:: ../lock/README.rst
   :start-after: matter_door_lock_sample_commissioning_start
   :end-before: matter_door_lock_sample_commissioning_end

.. _matter_thermostat_network_mode_onboarding:

Onboarding information
++++++++++++++++++++++

When you start the commissioning procedure, the controller must get the onboarding information from the Matter accessory device.
The onboarding information representation depends on your commissioner setup.

For this sample, you can use one of the following :ref:`onboarding information formats <ug_matter_network_topologies_commissioning_onboarding_formats>` to provide the commissioner with the data payload that includes the device discriminator and the setup PIN code:

  .. list-table:: Thermostat sample onboarding information
     :header-rows: 1

     * - QR Code
       - QR Code Payload
       - Manual pairing code
     * - Scan the following QR code with the app for your ecosystem:

         .. figure:: ../../../doc/nrf/images/matter_qr_code_thermostat.png
            :width: 200px
            :alt: QR code for commissioning the thermostat device

       - MT:O4CT342C00KA0648G00
       - 34970112332

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_onboarding_start
    :end-before: matter_door_lock_sample_onboarding_end

Upgrading the device firmware
=============================

To upgrade the device firmware, complete the steps listed for the selected method in the :doc:`matter:nrfconnect_examples_software_update` tutorial of the Matter documentation.

Dependencies
************

This sample uses the Matter library that includes the |NCS| platform integration layer:

* `Matter`_

In addition, the sample uses the following |NCS| components:

* :ref:`dk_buttons_and_leds_readme`

The sample depends on the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`
