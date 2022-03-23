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

If you want to commission the lock device and :ref:`control it remotely <matter_lock_sample_network_mode>` through a Thread network, you also need a Matter controller device :ref:`configured on PC or smartphone <ug_matter_configuring>`. This requires additional hardware depending on the setup you choose.

.. note::
    |matter_gn_required_note|

Overview
********

The sample uses buttons for changing the lock and device states, and LEDs to show the state of these changes.
You can test it in the following ways:

* Standalone, using a single DK that runs the door lock application.
* Remotely over the Thread protocol, which requires more devices.

The remote control testing requires a Matter controller that you can configure either on a PC or mobile device (for remote testing in a network).
You can enable both methods after :ref:`building and running the sample <matter_lock_sample_remote_control>`.

.. _matter_lock_sample_network_mode:

Remote testing in a network
===========================

.. matter_door_lock_sample_remote_testing_start

By default, the Matter accessory device has Thread disabled.
You must pair it with the Matter controller over Bluetooth® LE to get the configuration from the controller to use the device within a Thread network.
You have to make the device discoverable manually (for security reasons).
The controller must get the commissioning information from the Matter accessory device and provision the device into the network.
For details, see the `Commissioning the device`_ section.

.. matter_door_lock_sample_remote_testing_end

.. _matter_lock_sample_test_mode:

Remote testing using test mode
==============================

.. matter_door_lock_sample_test_mode_start

Alternatively to the commissioning procedure, you can use the test mode that allows joining the Thread network with default static parameters and static cryptographic keys.
|matter_sample_button3_note|

.. note::
    The test mode is not compliant with Matter and it only works together with the Matter controller and other devices that use the same default configuration.

.. matter_door_lock_sample_test_mode_end

Configuration
*************

|config|

Matter door lock build types
============================

.. matter_door_lock_sample_configuration_file_types_start

The sample uses different configuration files depending on the supported features.
Configuration files are provided for different build types and they are located in the :file:`configuration/<board_name>` directory.

The :file:`prj.conf` file represents a ``debug`` build type.
Other build types are covered by dedicated files with the build type added as a suffix to the ``prj`` part, as per the following list.
For example, the ``release`` build type file name is :file:`prj_release.conf`.
If a board has other configuration files, for example associated with partition layout or child image configuration, these follow the same pattern.

.. include:: /gs_modifying.rst
   :start-after: build_types_overview_start
   :end-before: build_types_overview_end

Before you start testing the application, you can select one of the build types supported by the sample.
This sample supports the following build types, depending on the selected board:

* ``debug`` -- Debug version of the application - can be used to enable additional features for verifying the application behavior, such as logs or command-line shell.
* ``release`` -- Release version of the application - can be used to enable only the necessary application functionalities to optimize its performance.
* ``no_dfu`` -- Debug version of the application without Device Firmware Upgrade feature support - can be used only for the nRF52840 DK and nRF5340 DK, as those platforms have DFU enabled by default.

.. note::
    `Selecting a build type`_ is optional.
    The ``debug`` build type is used by default if no build type is explicitly selected.

.. matter_door_lock_sample_configuration_file_types_end

Device Firmware Upgrade support
===============================

.. matter_door_lock_sample_build_with_dfu_start

.. note::
   You can enable over-the-air Device Firmware Upgrade only on hardware platforms that have external flash memory.
   Currently only nRF52840 DK and nRF5340 DK support Device Firmware Upgrade feature.

The sample supports over-the-air (OTA) device firmware upgrade (DFU) using one of the two following protocols:

* Matter OTA update protocol that uses the Matter operational network for querying and downloading a new firmware image.
* Simple Management Protocol (SMP) over Bluetooth® LE.
  In this case, the DFU can be done either using a smartphone application or a PC command line tool.
  Note that this protocol is not part of the Matter specification.

In both cases, MCUboot secure bootloader is used to apply the new firmware image.

The DFU over Matter is enabled by default.
To configure the sample to support the DFU over Matter and SMP, use the ``-DOVERLAY_CONFIG=../../overlay-smp_dfu.conf`` build flag during the build process.
To configure the sample to disable the DFU and the secure bootloader, use the ``-DCONF_FILE=prj_no_dfu.conf`` build flag during the build process.

See :ref:`cmake_options` for instructions on how to add these options to your build.

When building on the command line, run the following command with *build_target* replaced with the build target name of the hardware platform you are using (see `Requirements`_), and *dfu_build_flag* replaced with the desired DFU build flag:

.. parsed-literal::
   :class: highlight

   west build -b *build_target* -- *dfu_build_flag*

.. matter_door_lock_sample_build_with_dfu_end

FEM support
===========

.. include:: /includes/sample_fem_support.txt

Low-power build
===============

To configure the sample to consume less power, use the low-power build.
It enables Thread's Sleepy End Device mode and disables debug features, such as the UART console or the **LED 1** usage.

To trigger the low-power build, use the ``-DOVERLAY_CONFIG="../../overlay-low_power.conf"`` option when building the sample.
See :ref:`cmake_options` for instructions on how to add this option to your build.

When building on the command line, run the following command with *build_target* replaced with the build target name of the hardware platform you are using (see `Requirements`_):

.. parsed-literal::
   :class: highlight

   west build -b *build_target* -- -DOVERLAY_CONFIG="../../overlay-low_power.conf"

User interface
**************

.. matter_door_lock_sample_led1_start

LED 1:
    Shows the overall state of the device and its connectivity.
    The following states are possible:

    * Short Flash On (50 ms on/950 ms off) - The device is in the unprovisioned (unpaired) state and is waiting for a commissioning application to connect.
    * Rapid Even Flashing (100 ms on/100 ms off) - The device is in the unprovisioned state and a commissioning application is connected over Bluetooth LE.
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

   * If pressed for less than three seconds, it initiates the SMP server (Security Manager Protocol).
     After that the Direct Firmware Update (DFU) over Bluetooth Low Energy can be started. (see `Upgrading the device firmware`_).
   * If pressed for more than three seconds, it initiates the factory reset of the device.
     Releasing the button within the 3-second window cancels the factory reset procedure.

.. matter_door_lock_sample_button1_end

Button 2:
    Changes the lock state to the opposite one.

Button 3:
    Starts the Thread networking in the :ref:`test mode <matter_lock_sample_test_mode>` using the default configuration.

.. matter_door_lock_sample_button4_start

Button 4:
    Starts the NFC tag emulation, enables Bluetooth LE advertising for the predefined period of time (15 minutes by default), and makes the device discoverable over Bluetooth LE.
    This button is used during the :ref:`commissioning procedure <matter_lock_sample_remote_control_commissioning>`.

.. matter_door_lock_sample_button4_end

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

Selecting a build type
======================

Before you start testing the application, you can select one of the `Matter door lock build types`_, depending on your building method.

Selecting a build type in |VSC|
-------------------------------

.. include:: /gs_modifying.rst
   :start-after: build_types_selection_vsc_start
   :end-before: build_types_selection_vsc_end

Selecting a build type from command line
----------------------------------------

.. include:: /gs_modifying.rst
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

      File not found: ./ncs/nrf/samples/matter/lock/configuration/nrf52840dk_nrf52840/prj_shell.conf

Testing
=======

After building the sample and programming it to your development kit, complete the following steps to test its basic features:

#. |connect_kit|
#. |connect_terminal_ANSI|
#. Observe that **LED 2** is lit, which means that the door lock is closed.
#. Press **Button 2** to unlock the door.
   **LED 2** is blinking while the lock is opening.
   After approximately two seconds, **LED 2** turns off permanently.
   The following messages appear on the console:

   .. code-block:: console

      I: Unlock Action has been initiated
      I: Unlock Action has been completed

#. Press **Button 2** one more time to lock the door again.
   **LED 2** starts blinking and remains turned on.
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

Use one of the following to enable remote control:

* `Commissioning the device`_ allows you to set up a testing environment and remotely control the sample over a Matter-enabled Thread network.
* `Remote testing using test mode`_ allows you to test the sample functionalities in a Thread network with default parameters, without commissioning.
  |matter_sample_button3_note|

.. matter_door_lock_sample_remote_control_end

.. _matter_lock_sample_remote_control_commissioning:

Commissioning the device
------------------------

.. matter_door_lock_sample_commissioning_start

To commission the device, go to the :ref:`ug_matter_configuring_env` guide and complete the steps for the Matter controller you want to use.
The guide walks you through the following steps:

* Configure the Thread Border Router.
* Build and install the Matter controller.
* Commission the device.
* Send Matter commands that cover scenarios described in the `Testing`_ section.

If you are new to Matter, the recommended approach is :ref:`ug_matter_configuring_mobile` using an Android smartphone.

.. matter_door_lock_sample_commissioning_end

Upgrading the device firmware
=============================

To upgrade the device firmware, complete the steps listed for the selected method in the :doc:`matter:nrfconnect_examples_software_update` tutorial of the Matter documentation.

Dependencies
************

This sample uses the Matter library that includes the |NCS| platform integration layer:

* `Matter`_

In addition, the sample uses the following |NCS| components:

* :ref:`dk_buttons_and_leds_readme`
* :ref:`nfc_uri`
* :ref:`lib_nfc_t2t`

The sample depends on the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`
