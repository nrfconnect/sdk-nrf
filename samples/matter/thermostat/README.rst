.. _matter_thermostat_sample:

Matter: Thermostat
################

.. contents::
   :local:
   :depth: 2

This sample demonstrates a minimal implementation of the :ref:`Matter <ug_matter>` application layer.
This basic implementation enables the commissioning on the device, which allows it to join a Matter network built on top of a low-power, 802.15.4 Thread network or on top of a Wi-Fi network.
Support for both Thread and Wi-Fi is mutually exclusive and depends on the hardware platform, so only one protocol can be supported for a specific Matter device.
In case of Thread, this device works as a Thread :ref:`Minimal End Device <thread_ot_device_types>`.

Use this sample as a reference for developing your own application.
See the :ref:`ug_matter_creating_accessory` page for an overview of the process you need to follow.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

For testing purposes, that is to commission the device and :ref:`control it remotely <matter_thermostat_network_mode>` through a Thread network, you also need a Matter controller device :ref:`configured on PC or smartphone <ug_matter_configuring>`. This requires additional hardware depending on the setup you choose.

.. note::
    |matter_gn_required_note|

IPv6 network support
====================

The development kits for this sample offer the following IPv6 network support for Matter:

* Matter over Thread is supported for ``nrf52840dk_nrf52840``, ``nrf5340dk_nrf5340_cpuapp``, and ``nrf21540dk_nrf52840``.
* Matter over Wi-Fi is supported for ``nrf5340dk_nrf5340_cpuapp`` with the ``nrf7002_ek`` shield attached or for ``nrf7002dk_nrf5340_cpuapp``.

Overview
********

The sample starts the Bluetooth® LE advertising automatically and prepares the Matter device for commissioning into a Matter-enabled Thread network.
The sample uses an LED to show the state of the connection.
You can press a button to start the factory reset when needed.

.. _matter_thermostat_network_mode:

Remote testing in a network
===========================

Testing in either a Matter-enabled Thread or a Wi-Fi network requires a Matter controller that you can configure on PC or mobile device.
By default, the Matter accessory device has IPv6 networking disabled.
You must pair the device with the Matter controller over Bluetooth® LE to get the configuration from the controller to use the device within a Thread or a Wi-Fi network.
You can enable the controller after :ref:`building and running the sample <matter_thermostat_network_testing>`.

To pair the device, the controller must get the :ref:`matter_thermostat_network_mode_onboarding` from the Matter accessory device and commission the device into the network.


Commissioning in Matter
-----------------------

In Matter, the commissioning procedure takes place over Bluetooth LE between a Matter accessory device and the Matter controller, where the controller has the commissioner role.
When the procedure has completed, the device is equipped with all information needed to securely operate in the Matter network.

During the last part of the commissioning procedure (the provisioning operation), the Matter controller sends the Thread or Wi-Fi network credentials to the Matter accessory device.
As a result, the device can join the IPv6 network and communicate with other devices in the network.

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

       - MT:O4CT3KQS02ZZ 
       - 548G00

Configuration
*************x

|config|


.. _matter_light_switch_sample_acl:

Access Control List
===================

The Access Control List (ACL) is a list related to the Access Control cluster.
The list contains rules for managing and enforcing access control for a node's endpoints and their associated cluster instances.
In this sample's case, this allows the temperature measurement devices to receive messages from the thermostat and to get temperature data.

You can read more about ACLs on the :doc:`matter:access-control-guide` in the Matter documentation.

.. _matter_thermostat_sample_binding:

Binding
=======
.. include:: ../light_switch.README.rst
   :start-after: matter_light_switch_sample_binding_start
   :end-before: matter_light_switch_sample_binding_end

This sample can operate in two modes:

   mocked temperature sensor mode - the thermostat application generates the simulated temperature measurements
   real temperature sensor mode - the thermostat is bound to the remote Matter temperature sensor, which provides real temperature measurements

Matter thermostat build types
===========================

The sample uses different configuration files depending on the supported features.
Configuration files are provided for different build types and they are located in the application root directory.

The :file:`prj.conf` file represents a ``debug`` build type.
Other build types are covered by dedicated files with the build type added as a suffix to the ``prj`` part, as per the following list.
For example, the ``release`` build type file name is :file:`prj_release.conf`.
If a board has other configuration files, for example associated with partition layout or child image configuration, these follow the same pattern.

.. include:: /config_and_build/modifying.rst
   :start-after: build_types_overview_start
   :end-before: build_types_overview_end

Before you start testing the application, you can select one of the build types supported by the sample.
This sample supports the following build types, depending on the selected board:

* ``debug`` -- Debug version of the application - can be used to enable additional features for verifying the application behavior, such as logs or command-line shell.
* ``release`` -- Release version of the application - can be used to enable only the necessary application functionalities to optimize its performance.
* ``no_dfu`` -- Debug version of the application without Device Firmware Upgrade feature support - can be used for the nRF52840 DK, nRF5340 DK, nRF7002 DK, and nRF21540 DK.

.. note::
    `Selecting a build type`_ is optional.
    The ``debug`` build type is used by default if no build type is explicitly selected.

Device Firmware Upgrade support
===============================

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_build_with_dfu_start
    :end-before: matter_door_lock_sample_build_with_dfu_end

FEM support
===========

.. include:: /includes/sample_fem_support.txt

User interface
**************

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_led1_start
    :end-before: matter_door_lock_sample_led1_end

Button 1:
     If pushed Software update advertisement.
     If pressed for six seconds, it initiates the factory reset of the device.
     Releasing the button within the six-second window cancels the factory reset procedure.

Button 2:
     If pushed print thermostat data to the console.
     On nRF7002 dk if pressed for six seconds starts BLE advertisement 
Button 4:
      On nRF5340 and nRF52840 starts BLE advertisement

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_jlink_start
    :end-before: matter_door_lock_sample_jlink_end

Building and running
********************

.. |sample path| replace:: :file:`samples/matter/thermostat`

.. include:: /includes/build_and_run.txt

Selecting a build type
======================

Before you start testing the application, you can select one of the `Matter thermostat build types`_, depending on your building method.
Thermsotat sample has additional configuration which allows you to connect and test it with external matter sensor. For this option you need 
to select this option by adding the kconfig flag -DCONFIG_EXTERNAL_SENSOR=y. 

Selecting a build type in |VSC|
-------------------------------

.. include:: /getting_started/modifying.rst
   :start-after: build_types_selection_vsc_start
   :end-before: build_types_selection_vsc_end

Selecting a build type from command line
----------------------------------------

.. include:: /getting_started/modifying.rst
   :start-after: build_types_selection_cmd_start
   :end-before: For example, you can replace the

For example, you can replace the *selected_build_type* variable to build the ``release`` firmware for ``nrf52840dk_nrf52840`` by running the following command in the project directory:

.. parsed-literal::
   :class: highlight

   west build -b nrf52840dk_nrf52840 -d build_nrf52840dk_nrf52840 -- -DCONF_FILE=prj_release.conf

The ``build_nrf52840dk_nrf52840`` parameter specifies the output directory for the build files.

.. note::
   If the selected board does not support the selected build type, the build is interrupted.
   For example, if the ``shell`` build type is not supported by the selected board, the following notification appears:

   .. code-block:: console

      File not found: ./ncs/nrf/samples/matter/thermostat/configuration/nrf52840dk_nrf52840/prj_shell.conf



Testing
=======

When you have built the sample and programmed it to your development kit, it automatically starts the Bluetooth LE advertising and the **LED1** starts flashing (Short Flash On).
Thermostat should update its values every 30s and print data to logs.
By pressing **Button 2** you can print thermostat data to the logs.
At this point, you can press **Button 1** for six seconds to initiate the factory reset of the device.


.. _matter_thermostat_network_testing:

Testing in a network
--------------------

To test the sample in a Matter-enabled Thread network, complete the following steps:

.. matter_thermostat_sample_testing_start

1. |connect_kit|
#. |connect_terminal_ANSI|
#. Press **Button 2** to print Thermostat data to logs.
#. Press **Button 1** for six seconds to initiate the factory reset of the device.

The device reboots after all its settings are erased.


Testing with external sensor 
----------------------------

   Thermostat sample can be connected with external sensor device (for example Thingy:53: Matter weather station). Connection is done by :ref:`matter_binding` to matter temperature measurement cluster.
   
   .. note::
      To enable external sensor usage, you need to set CONFIG_EXTERNAL_SENSOR=y in .conf file
      or add -DCONFIG_EXTERNAL_SENSOR=y flag to build west command

1. |connect_kit_and_sensor_device|
#. |connect_terminal_ANSI|
#. Commission both devices into a Matter network by following the guides linked on the :ref:`ug_matter_configuring` page for the Matter controller you want to use.
   The guides walk you through the following steps:

   * Only if you are configuring Matter over Thread: Configure the Thread Border Router.
   * Build and install the Matter controller.
   * Commission the device.
     You can use the :ref:`matter_thermostat_network_mode_onboarding` listed earlier on this page.
   

#. Connect_thermostat_with_sensor
#. Use the :doc:`CHIP Tool <matter:chip_tool_guide>` ("Writing ACL to the ``accesscontrol`` cluster" section) to add proper ACL for the temperature device.

   * chip-tool accesscontrol write acl '[{"fabricIndex": 1, "privilege": 5, "authMode": 2, "subjects": [112233], "targets": null}, \
     {"fabricIndex": 1, "privilege": 1, "authMode": 2, "subjects": [<Thermostat NodeId>], "targets": \
     [{"cluster": 1026, "endpoint": <TemperatureSensor EndpointId>, "deviceType": null}]}]' <Sensor Device NodeId> 0

#. Use the :doc:`CHIP Tool <matter:chip_tool_guide>` ("Adding a binding table to the ``binding`` cluster) to add binding table to thermostat.

   * chip-tool binding write binding '[{"fabricIndex": 1, "node": <TemperatureSensor NodeId>, "endpoint": <TemperatureSensor EndpointId>, \
     "cluster": 1026}]' <Thermostat NodeId> <Thermostat EndpointId>

   After that thermostat should be able to read data from temperature sensor.


#. Press **Button 2** to print Thermostat data to logs.
#. Press **Button 1** for six seconds to initiate the factory reset of the device.

The device reboots after all its settings are erased.

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
``