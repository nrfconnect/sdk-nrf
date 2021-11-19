.. _matter_lock_sample:
.. _chip_lock_sample:

Matter: Door lock
#################

.. contents::
   :local:
   :depth: 2

This door lock sample demonstrates the usage of the :ref:`Matter <ug_matter>` (formerly Project Connected Home over IP, Project CHIP) application layer to build a door lock device with one basic bolt.
This device works as a Matter accessory device, meaning it can be paired and controlled remotely over a Matter network built on top of a low-power 802.15.4 Thread network.
You can use this sample as a reference for creating your application.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840, nrf5340dk_nrf5340_cpuapp, nrf21540dk_nrf52840

For remote testing scenarios, if you want to commission the lock device and :ref:`control it remotely <matter_lock_sample_network_mode>` through a Thread network, you also need a Matter controller device :ref:`configured on PC or smartphone <ug_matter_configuring>` (which requires additional hardware depending on which setup you choose).

.. note::
    |matter_gn_required_note|

Overview
********

The sample uses buttons for changing the lock and device states, and LEDs to show the state of these changes.
It can be tested in the following ways:

* Standalone, by using a single DK that runs the door lock application.
* Remotely over the Thread protocol, which requires more devices.

The remote control testing requires a Matter controller, which can be configured either on PC or mobile (for remote testing in a network).
Both methods can be enabled after :ref:`building and running the sample <matter_lock_sample_remote_control>`.

.. _matter_lock_sample_network_mode:

Remote testing in a network
===========================

.. matter_door_lock_sample_remote_testing_start

By default, the Matter accessory device has Thread disabled.
You must pair it with the Matter controller over Bluetooth® LE to get configuration from the controller if you want to use the device within a Thread network.
To do this, the device must be made discoverable manually (for security reasons) and the controller must get the commissioning information from the Matter accessory device and provision the device into the network.
For details, see the `Commissioning the device`_ section.

.. matter_door_lock_sample_remote_testing_end

.. _matter_lock_sample_test_mode:

Remote testing using test mode
==============================

.. matter_door_lock_sample_test_mode_start

Alternatively to the commissioning procedure, you can use the test mode, which allows to join a Thread network with default static parameters and static cryptographic keys.
|matter_sample_button3_note|

.. note::
    The test mode is not compliant with Matter and it only works together with Matter controller and other devices which use the same default configuration.

.. matter_door_lock_sample_test_mode_end

Configuration
*************

|config|

.. matter_door_lock_sample_configuration_file_types_start

The sample uses different configuration files depending on the supported features.
The configuration files are automatically attached to the build based on the build type suffix of the file name, such as :file:`_single_image_dfu` in the :file:`prj_single_image_dfu.conf` file.
To modify configuration options, apply the modifications to the files with the appropriate suffix.
See the table below for information about available configuration types:

+---------------------------+--------------------------------------+------------------------+------------------------------+
| Config suffix             | Enabled feature                      | Enabling build option  | Supported boards             |
+===========================+======================================+========================+==============================+
| none                      | none (basic build)                   | none                   | ``nrf52840dk_nrf52840``      |
|                           |                                      |                        | ``nrf5340dk_nrf5340_cpuapp`` |
+---------------------------+--------------------------------------+------------------------+------------------------------+
| :file:`_single_image_dfu` | Single-image Device Firmware Upgrade | ``-DBUILD_WITH_DFU=1`` | ``nrf52840dk_nrf52840``      |
+---------------------------+--------------------------------------+------------------------+------------------------------+
| :file:`_multi_image_dfu`  | Multi-image Device Firmware Upgrade  | ``-DBUILD_WITH_DFU=1`` | ``nrf5340dk_nrf5340_cpuapp`` |
+---------------------------+--------------------------------------+------------------------+------------------------------+

.. matter_door_lock_sample_configuration_file_types_end

Device Firmware Upgrade support
===============================

.. matter_door_lock_sample_build_with_dfu_start

.. note::
   Over-the-air Device Firmware Upgrade can be enabled only on hardware platforms containing external flash memory.

You can configure the sample to use the secure bootloader for performing over-the-air Device Firmware Upgrade using Bluetooth® LE, using the ``-DBUILD_WITH_DFU=1`` build flag during the build process.

See :ref:`cmake_options` for instructions on how to add these options to your build.

For example, when building on the command line, you can run the following command with *build_target* replaced with the build target name of the hardware platform you are using (see `Requirements`_):

.. parsed-literal::
   :class: highlight

   west build -b *build_target* -- -DBUILD_WITH_DFU=1

.. matter_door_lock_sample_build_with_dfu_end

FEM support
===========

.. include:: /includes/sample_fem_support.txt

Low-power build
===============

You can configure the sample to consume less power by using the low-power build, which enables Thread's Sleepy End Device mode and disables debug features, such as the UART console or the **LED 1** usage.

To trigger the low-power build, use the ``-DOVERLAY_CONFIG="overlay-low_power.conf"`` option when building the sample.
See :ref:`cmake_options` for instructions on how to add this option to your build.

For example, when building on the command line, you can run the following command with *build_target* replaced with the build target name of the hardware platform you are using (see `Requirements`_):

.. parsed-literal::
   :class: highlight

   west build -b *build_target* -- -DOVERLAY_CONFIG="overlay-low_power.conf"

User interface
**************

.. matter_door_lock_sample_led1_start

LED 1:
    Shows the overall state of the device and its connectivity.
    The following states are possible:

    * Short Flash On (50 ms on/950 ms off) - The device is in the unprovisioned (unpaired) state and is waiting for a commissioning application to connect.
    * Rapid Even Flashing (100 ms on/100 ms off) - The device is in the unprovisioned state and a commissioning application is connected through Bluetooth LE.
    * Solid On - The device is fully provisioned.

.. matter_door_lock_sample_led1_end

LED 2:
    Shows the state of the lock.
    The following states are possible:

    * Solid On - The bolt is extended and the door is locked.
    * Off - The bolt is retracted and the door is unlocked.
    * Rapid Even Flashing (50 ms on/50 ms off during 2 s) - The simulated bolt is in motion from one position to another.

.. matter_door_lock_sample_button1_start

Button 1:
    Depending on how long you press the button:

    * If pressed for 6 seconds, it initiates the factory reset of the device.
      Releasing the button within the 6-second window cancels the factory reset procedure.
    * If pressed for less than 3 seconds, it initiates the OTA software update process.
      The OTA process is disabled by default, but you can enable it when you build the sample with the DFU support (see `Configuration`_).

.. matter_door_lock_sample_button1_end

Button 2:
    Changes the lock state to the opposite one.

Button 3:
    Starts the Thread networking in the :ref:`test mode <matter_lock_sample_test_mode>` using the default configuration.

Button 4:
    Starts the NFC tag emulation, enables Bluetooth LE advertising for the predefined period of time (15 minutes by default), and makes the device discoverable over Bluetooth LE.
    This button is used during the :ref:`commissioning procedure <matter_lock_sample_remote_control_commissioning>`.

.. matter_door_lock_sample_jlink_start

SEGGER J-Link USB port:
    Used for getting logs from the device or for communicating with it through the command-line interface.

.. matter_door_lock_sample_jlink_end

NFC port with antenna attached:
    Optionally used for obtaining the commissioning information from the Matter accessory device to start the :ref:`commissioning procedure <matter_lock_sample_remote_control>`.

Building and running
********************

.. |sample path| replace:: :file:`samples/matter/lock`

.. include:: /includes/build_and_run.txt

See `Configuration`_ for information about building the sample with the DFU support.

Testing
=======

After building the sample and programming it to your development kit, test its basic features by performing the following steps:

#. |connect_kit|
#. |connect_terminal|
#. Observe that initially **LED 2** is lit, which means that the door lock is closed.
#. Press **Button 2** to unlock the door.
   **LED 2** is blinking while the lock is opening.
   After approximately 2 seconds, **LED 2** turns off permanently.
   The following messages appear on the console:

   .. code-block:: console

      I: Unlock Action has been initiated
      I: Unlock Action has been completed

#. Press **Button 2** one more time to lock the door again.
   **LED 2** starts blinking and then remains turned on.
   The following messages appear on the console:

   .. code-block:: console

      I: Lock Action has been initiated
      I: Lock Action has been completed

#. Press **Button 1** to initiate factory reset of the device.

The device reboots after all its settings are erased.

.. _matter_lock_sample_remote_control:

Enabling remote control
=======================

Remote control allows you to control the Matter door lock device from a Thread network.

.. matter_door_lock_sample_remote_control_start

You can use one of the following options to enable this option:

* `Commissioning the device`_, which allows you to set up testing environment and remotely control the sample over a Matter-enabled Thread network.
* `Remote testing using test mode`_, which allows you to test the sample functionalities in a Thread network with default parameters, without commissioning.
  |matter_sample_button3_note|

.. matter_door_lock_sample_remote_control_end

.. _matter_lock_sample_remote_control_commissioning:

Commissioning the device
------------------------

.. matter_door_lock_sample_commissioning_start

To commission the device, go to the :ref:`ug_matter_configuring_env` page and complete the steps for the Matter controller you want to use.
As part of this tutorial, you will configure Thread Border Router, build and install the Matter controller, commission the device, and send Matter commands that cover scenarios described in the `Testing`_ section.
If you are new to Matter, the recommended approach is :ref:`ug_matter_configuring_mobile` using an Android smartphone.

.. matter_door_lock_sample_commissioning_end

Upgrading the device firmware
=============================

To upgrade the device firmware, complete the steps listed for the selected method in the :doc:`matter:nrfconnect_examples_software_update` tutorial in the Matter documentation.

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
