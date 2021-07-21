.. _matter_weather_station_sample:

Matter: Weather station
#######################

.. contents::
   :local:
   :depth: 2

This weather station sample demonstrates the usage of the :ref:`Matter <ug_matter>` (formerly Project Connected Home over IP, Project CHIP) application layer to build a weather station device that lets you remotely gather different kinds of data, such as temperature, air pressure, and relative humidity, using the device sensors.
This device works as a Matter accessory device, meaning it can be paired and controlled remotely over a Matter network built on top of a low-power, 802.15.4 Thread network.
You can use this sample as a reference for creating your own application.

.. note::
    |weather_station_wip_note|

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: thingy53_nrf5340_cpuapp

To program and gather logs from the Thingy:53 device, you need the external J-Link programmer.
If you own an nRF5340 DK that has an onboard J-Link programmer, you can also use it for this purpose.

To commission the weather station device and :ref:`control it remotely <matter_weather_station_sample_remote_control>` through a Thread network, you also need the following:

* A Matter controller device :ref:`configured on PC or smartphone <ug_matter_configuring>` (which requires additional hardware depending on which setup you choose).

.. note::
    |matter_gn_required_note|

Overview
********

The sample uses a single button for controlling the device state.
The weather station device is periodically performing temperature, air pressure, and relative humidity measurements.
The measurement results are stored in the device memory and can be read using the Matter controller.
The controller communicates with the weather station device over the Matter protocol using Zigbee Cluster Library (ZCL).
The library describes data measurements within the proper clusters that correspond to the measurement type.

The sample can be tested remotely over the Thread protocol, which requires an additional Matter controller device that can be configured either on PC or mobile (for remote testing in a network).
You can start testing after :ref:`building and running the sample <matter_weather_station_sample_remote_control>`.

.. _matter_weather_station_network_mode:

Remote testing in a network
===========================

By default, the Matter accessory device has Thread disabled, and it must be paired with the Matter controller over Bluetooth LE to get configuration from it if you want to use the device within a Thread network.
To do this, the device must be made discoverable over Bluetooth LE that starts automatically upon the device startup but only for a predefined period of time (15 minutes by default).
If the Bluetooth LE advertising times out, you can re-enable it manually using **Button 1**.
Additionally, the controller must get the commissioning information from the Matter accessory device and provision the device into the network.
For details, see the `Commissioning the device`_ section.

Configuration
*************

|config|

.. _matter_weather_station_sample_selecting_build_configuration:

Selecting build configuration
=============================

This sample supports the following build types:

* Debug - This build type enables additional features for verifying the application behavior, such as logs or command-line shell.
* Release - This build type enables only the necessary application functionalities to optimize its performance.

You can find configuration options for both build types in the corresponding :file:`prj_debug.conf` and :file:`prj_release.conf` files.
By default, the sample selects the release configuration.

If you want to build the target with the debug configuration, you can set the following flag when building the sample:

* ``-DCMAKE_BUILD_TYPE=debug``

See :ref:`cmake_options` for instructions on how to add these options to your build.
For example, when building on the command line, you can build the target with the debug configuration running the following command:

.. parsed-literal::
   :class: highlight

   west build -b thingy53_nrf5340_cpuapp -- -DCMAKE_BUILD_TYPE=debug

Device Firmware Upgrade support
===============================

This sample by default uses MCUboot for performing over-the-air Device Firmware Upgrade using Bluetooth LE.
To build the sample with both the secure bootloader and the DFU support disabled, you can use the ``-DBUILD_WITH_DFU=0`` flag in your build.

See :ref:`cmake_options` for instructions on how to add this option to your build.
For example, when building on the command line, you can run the following command:

.. parsed-literal::
   :class: highlight

   west build -b thingy53_nrf5340_cpuapp -- -DBUILD_WITH_DFU=0

User interface
**************

LED 1:
    Shows the overall state of the device and its connectivity.
    The following states are possible:

    * Green color short flash on (50 ms on/950 ms off) - The device is in the unprovisioned (unpaired) state and is not advertising over Bluetooth LE.
    * Blue color short flash on (50 ms on/950 ms off) - The device is in the unprovisioned (unpaired) state and is advertising over Bluetooth LE.
    * Blue color rapid even flashing (100 ms on/100 ms off) - The device is in the unprovisioned state and a commissioning application is connected through Bluetooth LE.
    * Purple color short flash on (50 ms on/950 ms off) - The device is fully provisioned and has Thread enabled.

Button 1:
    This button is used during the :ref:`commissioning procedure <matter_weather_station_sample_remote_control_commissioning>`.
    Depending on how long you press the button:

    * If pressed for 6 seconds, it initiates the factory reset of the device.
      Releasing the button within the 6-second window cancels the factory reset procedure.
    * If pressed for less than 3 seconds, it starts the NFC tag emulation, enables Bluetooth LE advertising for the predefined period of time, and makes the device discoverable over Bluetooth LE.

Serial Wire Debug port:
    Used for getting logs from the device or communicating with it through the command-line interface.
    It is enabled only for the debug configuration of a sample.
    See the :ref:`build configuration <matter_weather_station_sample_selecting_build_configuration>` to learn how to select debug configuration.

NFC port with antenna attached:
    Used for obtaining the commissioning information from the Matter accessory device to start the :ref:`commissioning procedure <matter_weather_station_sample_remote_control_commissioning>`.

Building and running
********************

.. |sample path| replace:: :file:`samples/matter/weather_station`

.. include:: /includes/build_and_run.txt

Testing
=======

.. note::
    |weather_station_wip_note|

.. _matter_weather_station_sample_remote_control:

Enabling remote control
=======================

Remote control allows you to control the Matter weather station device from a Thread network.
The weather station sample supports `Commissioning the device`_, which allows you to set up a testing environment and remotely control the sample over a Matter-enabled Thread network.

.. _matter_weather_station_sample_remote_control_commissioning:

Commissioning the device
------------------------

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_commissioning_start
    :end-before: matter_door_lock_sample_commissioning_end

Before starting the commissioning procedure, the device must be made discoverable over Bluetooth LE that starts automatically upon the device startup but only for a predefined period of time (15 minutes by default).
If the Bluetooth LE advertising times out, you can re-enable it manually using **Button 1**.

To start commissioning, the controller must get the commissioning information from the Matter accessory device.
The data payload, which includes the device discriminator and setup PIN code, is encoded and shared using an NFC tag.
When using the debug configuration, you can also get this type of information from the RTT interface logs.

Upgrading the device firmware
=============================

To upgrade the device firmware, complete the steps listed for the selected method in the `Performing Device Firmware Upgrade in Matter device`_ tutorial.

Dependencies
************

This sample uses the Matter library, which includes the |NCS| platform integration layer:

* `Matter`_

In addition, the sample uses the following |NCS| components:

* :ref:`dk_buttons_and_leds_readme`
* :ref:`nfc_uri`
* :ref:`lib_nfc_t2t`

The sample depends on the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`

.. |weather_station_wip_note| replace:: This sample is still in development and offers no testing scenarios at the moment.
